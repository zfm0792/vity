#include <atomic>
#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
namespace vity{

static Logger::ptr g_logger = VITY_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

static thread_local Fiber* t_fiber = nullptr; // 正在执行着的协程
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size = 
    Config::Lookup<uint32_t>("fiber.stack_size",1024*1024,"fiber stack size");

class MallocStackAllocator{
public:
    static void* Alloc(size_t size){
        return malloc(size);
    }
    static void Dealloc(void* ptr,size_t size){
        return free(ptr);
    }
};

using StackAllocator = MallocStackAllocator;

Fiber::Fiber(){
    m_state = EXEC;
    SetThis(this); // 给main_fiber进行赋值  线程局部变量的t_fiber
    // 主协程的上下文 的状态
    if(getcontext(&m_ctx)){
        VITY_ASSERT2(false,"getcontext");
    }
    ++s_fiber_count;

    VITY_LOG_DEBUG(g_logger) << "Fiber::Fiber()" << " m_id = " << m_id;
}
// 设置当前协程
void Fiber::SetThis(Fiber *f){
    t_fiber = f;
}
// 返回当前协程
// 协程的处理方式是: main_fiber ---- sub_fiber(执行完后 会被切换会main_fiber)  然后再由main_fiber进行重新处理
// 返回的是线程局部变量  线程中的main_fiber
Fiber::ptr Fiber::GetThis(){
    if(t_fiber){ // t_fiber 线程局部变量 如果有值   子线程 或者 主线程
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    t_threadFiber = main_fiber; // t_threadFiber 主线程的协程
    VITY_ASSERT(t_fiber == main_fiber.get());
    return t_fiber->shared_from_this();
}

Fiber::Fiber(std::function<void()> cb,size_t stacksize,bool use_caller)
    :m_id(++s_fiber_id)
    ,m_cb(cb)
    {
    ++s_fiber_count;
    // statcksize  = 0  采用默认大小  一般是采用默认大小
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    m_stack = StackAllocator::Alloc(m_stacksize);

    if(getcontext(&m_ctx)){
        VITY_ASSERT2(false,"getcontext");
    }
    // 此时是子协程
    m_ctx.uc_link = nullptr; // 设定后继的上下文
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    //m_ctx.uc_stack.ss_flags = 0;
    // 修改上下文指向   等待swap 激活
    if(!use_caller){
        makecontext(&m_ctx,&Fiber::MainFunc,0);
    }else{
        makecontext(&m_ctx,&Fiber::CallerMainFunc,0);
    }
   
    VITY_LOG_DEBUG(g_logger) << "Fiber::Fiber id= " << m_id;
}

Fiber::~Fiber(){
    --s_fiber_count;
    if(m_stack){ 
        VITY_ASSERT(m_state == INIT
            || m_state == EXCEPT
            || m_state == TERM);
        StackAllocator::Dealloc(m_stack,m_stacksize);
    }else{
        VITY_ASSERT(!m_cb);
        VITY_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if(cur == this){
            SetThis(nullptr);
        }
    }
    VITY_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id;

}

// 重置协程函数 并重置状态
void Fiber::reset(std::function<void()> cb)
{
    VITY_ASSERT(m_stack);
    VITY_ASSERT(m_state == INIT
        || m_state == EXCEPT
        || m_state == TERM);
    
    m_cb = cb;
    if(getcontext(&m_ctx)){
        VITY_ASSERT2(false,"getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    m_ctx.uc_stack.ss_flags = 0;

    makecontext(&m_ctx,&Fiber::MainFunc,0);
    m_state = INIT;
}

void Fiber::call(){
    SetThis(this); // 切换t_fiebr为当前执行者的fiber
    m_state = EXEC;
    VITY_LOG_ERROR(g_logger) << getId();
    if(swapcontext(&t_threadFiber->m_ctx,&m_ctx)){ // t_threadFiber 才是真正的主线程的main_fiber
        VITY_ASSERT2(false,"swapcontext");
    }
}

void Fiber::back(){
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx,&t_threadFiber->m_ctx)){
        VITY_ASSERT2(false,"swapcontext");
    }
}

void Fiber::MainFunc()
{
    Fiber::ptr cur = GetThis();
    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }catch(std::exception& ex){
        cur->m_state = EXCEPT;
        VITY_LOG_ERROR(g_logger) << "Fiber Except:" << ex.what()
            << " fiber id= " << cur->getId()
            << std::endl
            << vity::BacktraceToString();
    }catch(...){
        cur->m_state = EXCEPT;
        VITY_LOG_ERROR(g_logger) << "Fiber Except:"
            << " fiber id= " << cur->getId()
            << std::endl
            << vity::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();
    VITY_ASSERT2(false,"never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}
// 主线程的协程
void Fiber::CallerMainFunc()
{
    Fiber::ptr cur = GetThis();
    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }catch(std::exception& ex){
        cur->m_state = EXCEPT;
        VITY_LOG_ERROR(g_logger) << "Fiber Except:" << ex.what()
            << " fiber id= " << cur->getId()
            << std::endl
            << vity::BacktraceToString();
    }catch(...){
        cur->m_state = EXCEPT;
        VITY_LOG_ERROR(g_logger) << "Fiber Except:"
            << " fiber id= " << cur->getId()
            << std::endl
            << vity::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();
    VITY_ASSERT2(false,"never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

//切换到当前协程
void Fiber::swapIn()
{
    // 给t_fiber赋值
    SetThis(this);
    VITY_ASSERT(m_state != EXEC);
    m_state = EXEC; // 改变状态
    // 切换上下文  --- Get到了t_fiber
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx,&m_ctx)){
        VITY_ASSERT2(false,"swapcontext");
    }
}
// 切换到后台执行
void Fiber::swapOut(){
    SetThis(t_fiber);
    if(swapcontext(&m_ctx,&Scheduler::GetMainFiber()->m_ctx)){
        VITY_ASSERT2(false,"swapcontext");
    }
}

// 协程切换到后台 并设置为Ready状态
void Fiber::YieldToReady(){
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}

// 协程切换到后台 并设置为Hold状态
void Fiber::YieldToHold(){
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}

// 总协程数
uint64_t Fiber::TotalFibers(){
    return s_fiber_count;
}

uint64_t Fiber::GetFiberId(){
    if(t_fiber){
        return t_fiber->getId();
    }
    return 0;
}

}