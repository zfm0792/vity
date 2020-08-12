#ifndef __VITY_FIBER_H__
#define __VITY_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"

namespace vity{

class Fiber : public std::enable_shared_from_this<Fiber>{
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;
    // 协程的状态 
    enum State{
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };

private:
    // 主协程
    Fiber();

public:
    // 子协程
    Fiber(std::function<void()> cb,size_t stacksize = 0,bool use_call = false);
    ~Fiber();

    // 重置协程函数
    // INIT TERM
    void reset(std::function<void()> cb);
    // 切换到当前协程
    void swapIn();
    // 切换到出协程
    void swapOut();

    void call();
    void back();
    // 获取协程id
    uint64_t getId() const { return m_id; }
    // 获取协程状态
    State getState() const { return m_state; }

public:
    // 设置当前协程 给线程局部变量赋值 表明是哪个协程在执行 
    static void SetThis(Fiber *f);
    // 获取当前的协程  线程局部变量
    static Fiber::ptr GetThis();
    // 切换到后台 并设置为Ready状态
    static void YieldToReady();
    // 切换到后台 并设置为Hold状态
    static void YieldToHold();
    // 总协程数
    static uint64_t TotalFibers();

    static void MainFunc();
    static void CallerMainFunc();
    // 获取协程id
    static uint64_t GetFiberId();
private:

    // 协程id
    uint64_t m_id = 0;
    // 协程栈大小
    uint32_t m_stacksize;
    State m_state = INIT;

    ucontext_t m_ctx;
    // 协程栈空间
    void *m_stack = nullptr;
    // 协程执行的回调函数
    std::function<void()> m_cb;

};

}

#endif