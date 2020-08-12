#ifndef __VITY_SCHEDULER_H__
#define __VITY_SCHEDULER_H__

#include <memory>
#include <list>
#include <string>
#include <vector>
#include "thread.h"
#include "fiber.h"

namespace vity{

// 协程调度器
// M个线程  每个线程管理N协程
// M  --------  N
// 该调度器要同时管理线程和协程 ---- 是一个虚基类
//                                                      thread ------ fiber(M)
// 我们采用的方式是 ----                                  |   
// main ---- create threadPool ---- threadPool(N 个线程) |
class Scheduler{

public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(size_t threads = 1,bool use_caller = true,const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const { return m_name; }

    static Scheduler* GetThis();
    static Fiber* GetMainFiber();

    void start();
    void stop();

    // 将协程或者回调加入到消息队列中
    template<class FiberOrCb>
    void schedule(FiberOrCb fc,int thread = -1)
    {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc,thread);
        }

        if(need_tickle){
            tickle();
        }
    }
    
    // 批量将协程或者回调加入到消息队列中
    template<class InputIterator>
    void schedule(InputIterator begin,InputIterator end){
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin != end){
                need_tickle = scheduleNoLock(&*begin) || need_tickle;
            }
        }
        if(need_tickle){
            tickle();
        }
    }

protected:
    virtual void tickle();
    virtual bool stopping();
    virtual void idle();

    void run();
    void setThis();
    bool hasIdleThreads() {  return m_idleThreadCount > 0; }

private:
    // 将待调度的协程 或者回调函数加入到 待调用的队列中
    // thread: 线程id  fc 协程或者回调函数
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc,int thread)
    {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc,thread);
        if(ft.fiber || ft.cb){
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

private:
    struct FiberAndThread{
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        FiberAndThread(Fiber::ptr f,int thr)
            :fiber(f),thread(thr){

        }
        FiberAndThread(Fiber::ptr *f,int thr)
            :thread(thr){
                fiber.swap(*f);
        }
        FiberAndThread(std::function<void()> f,int thr)
            :cb(f),thread(thr){

        }
        FiberAndThread(std::function<void()> *f,int thr)
            :thread(thr){
                cb.swap(*f);
        }
        FiberAndThread()
            :thread(-1)
        {
        }
        void reset(){
            fiber =  nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads; // 待管理的N个线程   线程会产生协程  进行调度
    // 协程 --- 正在执行的函数
    // 回调(函数对象) --- 还没有执行的  待执行(INIT状态的协程) 
    std::list<FiberAndThread> m_fibers; // 待调度的线程或者协程 链表   fiber callback threadId  线程 管理调度协程或者还没有执行的函数
    Fiber::ptr m_rootFiber; // 主线程的协程
    // 调度器名称
    std::string m_name;

protected:
    // 保存所有的线程id
    std::vector<int> m_threadIds;
    // 线程总数
    size_t m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount { 0 }; // 活跃的线程数
    std::atomic<size_t> m_idleThreadCount { 0 }; // 空闲线程数
    bool m_stopping = true;
    bool m_autoStop = false;
    int m_rootThread = 0; // 主线程id

};

}

#endif