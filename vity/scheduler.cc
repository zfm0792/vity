#include "log.h"
#include "scheduler.h"
#include "macro.h"
namespace vity{

static vity::Logger::ptr g_logger = VITY_LOG_NAME("system");

// 定义两个线程局部变量 线程局部变量 是归该线程本身所拥有  其他线程是不可以访问
static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_fiber = nullptr;
// 构造函数被主线程所调用  构建对象
// 其他线程调用该函数  会出现怎样的效果???
// 该类的内部维护着一个线程池  最好只能由主线程所调用
//  Scheduler(size_t threads = 1,bool use_caller = true,const std::string& name = "");
Scheduler::Scheduler(size_t threads,bool use_caller,const std::string& name)
    :m_name(name)
{
    VITY_ASSERT(threads > 0);
    // 将自身加入到该调度器中进行调度
    // 将主线程的协程加入到该调度中
    if(use_caller){
        vity::Fiber::GetThis(); // 主线程的协程
        --threads; // 主线程 在被调度  因此 创建的线程数减少1  只有一个线程在被调度

        VITY_ASSERT(GetThis() == nullptr); // 如果主线程的协程创建失败  则直接退出
        t_scheduler = this; // 主线程中的 协程 调度器  主调度器
        // 主线程中一个协程  run
        // run 进行调度   指明协程的一个函数
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run,this),0,true)); // run 最终的协程调度
        vity::Thread::SetName(m_name); // 设置线程名称
        // t_fiber  t_scheduler 是线程局部对象  所以 可以说t_fiber   t_scheduler 是线程的主协程或者主协程调度
        // 在调度过程中  会将上下文在 此上下切换
        t_fiber = m_rootFiber.get();
        // 获取线程id
        m_rootThread = vity::GetThreadId();
        // m_threadIds 记录所有的线程id
        m_threadIds.push_back(m_rootThread);
    }
    else{
        m_rootThread = -1; // 主线程id -1  表示不归我们管理
    }
    m_threadCount = threads;
}
// 调用了析构 就表明 将要结束
Scheduler::~Scheduler()
{
    VITY_ASSERT(m_stopping);
    if(GetThis() == this){
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis()
{
    return t_scheduler;
}
// 利用线程局部对象 从而区分不同线程的mainFiber
Fiber* Scheduler::GetMainFiber()
{
    return t_fiber;
}
// 线程池 每个线程都会去到调度器中的run方法中被调度
void Scheduler::start()
{
    MutexType::Lock lock(m_mutex);
    // m_stopping == false 
    if(!m_stopping){ // 不是停止状态
        return ;
    }

    m_stopping = false; // 运行状态

    VITY_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount;++i){
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run,this)
            ,m_name + "_" + std::to_string(i)));
        // 在线程创建开始时构造函数内有一个信号量 sem 
        // 调用sem.wait() ---- run 结束后  sem.notify()  从而达到每个线程都能创建完毕
        m_threadIds.push_back(m_threads[i]->getId());
    }
}
// 保证所有协程正常退出  保证所有线程正常退出
// 由主线程所调用
void Scheduler::stop()
{
    // 当主线程调用stop的时候  就表示 要自动退出
    m_autoStop = true;
    // 检查线程数量 主线程的协程 以及各个状态
    if(m_rootFiber && m_threadCount == 0 && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT))
    {
        //
        VITY_LOG_INFO(g_logger) << this << "stopped";
        m_stopping = true;

        if(stopping()){
            return ;
        }
    }

    if(m_rootThread != -1){
        VITY_ASSERT(GetThis() == this);
    }else{
        VITY_ASSERT(GetThis() != this);
    }

    m_stopping = true;

    for(size_t i = 0; i < m_threadCount;++i){
        tickle();
    }
    
    if(m_rootFiber){
        tickle();
    }

    if(m_rootFiber){
        if(!stopping()){
            m_rootFiber->call(); // 回收资源使用
        }
    }

    // 
    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    // 等待所有的线程结束
    for( auto &i : thrs){
        i->join();
    }
}
// 一定是子线程先执行进来 而非主线程中的协程函数进来
// 在构造函数中 我们只是注册进来了  并没有swapIn 进入到此处
// 所以我们可以采用主线程中的 m_rootFiber对资源进行回收
void Scheduler::run()
{
    VITY_LOG_INFO(g_logger) << "run";
    setThis();
    // 不是主线程的协程  线程池中线程进来了
    // 主线程中的线程局部变量在最开始的时候已经被赋值了(构造函数)
    // m_rootThread: 主线程线程id
    if(vity::GetThreadId() != m_rootThread){
        t_fiber = Fiber::GetThis().get();// 给子线程创建一个协程
    }
    // 空闲协程执行的函数
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle,this)));
    Fiber::ptr cb_fiber;// 协程的回调

    // 从调度队列里面去取相应的协程 进行调度
    FiberAndThread ft;

    while(true){
        ft.reset(); // 重置
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            // it  ---- callback  fiber threadId
            while(it != m_fibers.end()){
                if(it->thread != -1 && it->thread != vity::GetThreadId()){
                    ++it;
                    tickle_me = true;
                    continue;
                }
                 // 如果it->thread 有值   说明 fiberOrCb是存在的  就需要去执行
                 VITY_ASSERT(it->fiber || it->cb);
                 // 如果该协程是执行中的  那么就继续下一个
                 if(it->fiber && it->fiber->getState() == Fiber::EXEC){
                     ++it;
                     continue;
                 }
                 // 是一个协程 但是状态不是EXEC  待处理(调度 改变运行状态)
                 ft = *it;
                 m_fibers.erase(it);
                 ++m_activeThreadCount;
                 is_active = true;
                 break;
            }
        }

        if(tickle_me){
            tickle();
        }
        // 取出来的协程不是TERM 以及EXCEPT 那么就去调度执行  改变其状态 切进去执行
        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
            && ft.fiber->getState() != Fiber::EXCEPT)){

                ft.fiber->swapIn();
                --m_activeThreadCount;

                // 切进去后 那里面变为READY
                if(ft.fiber->getState() == Fiber::READY){
                    schedule(ft.fiber); // 加入到scheduler调度中
                }else if(ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT){
                        ft.fiber->m_state = Fiber::HOLD;
                }
                ft.reset();
        }else if(ft.cb){
            if(cb_fiber){ // 不为空 则重置该协程中的执行体
                cb_fiber->reset(ft.cb); // 重置协程函数 并重置状态
            }else{ // 如果为空 则new一个新的协程给cb_fiber
                //首先生成新对象，然后引用计数减1，引用计数为0，故析构Fiber
                //最后将新对象的指针交给智能指针
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset(); // 将内容清空
            cb_fiber->swapIn(); // EXEC ft.cb
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY){
                schedule(cb_fiber);
                cb_fiber.reset();
            }else if(cb_fiber->getState() == Fiber::EXCEPT
                || cb_fiber->getState() == Fiber::TERM){
                    cb_fiber->reset(nullptr);
            }else{
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        }else{ 
            // 既不是回调 也不是 fiber  说明是空闲的
            if(is_active){
                --m_activeThreadCount;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM){
                VITY_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }
            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT){
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

void Scheduler::tickle()
{
    VITY_LOG_INFO(g_logger) << "tickle";
}
bool Scheduler::stopping()
{
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}
void Scheduler::idle()
{
    VITY_LOG_INFO(g_logger) << "idle";
    while(!stopping()){
        // 改变状态 Hold  并且swapOut
        vity::Fiber::YieldToHold();
    }
}

void Scheduler::setThis()
{
    t_scheduler = this;
}

}
