#ifndef __VITY_IOMANAGER_H__
#define __VITY_IOMANAGER_H__

#include "scheduler.h"
#include <vector>
#include <sys/epoll.h>

namespace vity{

class IOManager : public Scheduler{

public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event{
        NONE = 0x0,
        READ = 0x1,
        WRITE = 0x4
    };

private:
/*
    以前的结构:
    struct channel{  // 事件
        int fd;     // 触发事件的描述符
        int events; // 事件的类型
        // 事件回调
        event_read_callback eventReadCallBack;
        event_write_callback eventWriteCallBack;

        void * data;//callback data, 可能是event_loop，也可能是tcp_server或者tcp_connection
    };
*/

    // 事件 --- 文件描述符 --- 读 写  回调
    struct FdContext{
        typedef Mutex MutexType;
        // 采用多线程的方式  是单纯的回调函数
        // 采用协程  所以需要回调函数加协程调度部分 --- swapIn swapOut
        struct EventContext{
            Scheduler *scheduler = nullptr; // 事件执行的scheduler
            Fiber::ptr fiber;               // 事件的协程
            std::function<void()> cb;       // 事件的回调
        };
        // 获取事件的上下文  回调  根据event类型来返回相应的fdContext
        EventContext& getContext(Event event);
        // 重置 ctx->scheduler = nullptr ctx->fiber.reset() ctx->cb = nullptr
        void resetContext(EventContext& ctx);
        // 触发 事件   直接调用回调函数  现在是采用 fd_ctx = getContext()
        // fd_ctx.scheduler->scheduler(fd_ctx.cb);
        // fd_ctx.scheduler->scheduler(fd_ctx.fiber);
        void triggerEvent(Event event);

        int fd;             // 事件关联的句柄
        EventContext read;  // 读事件  协程  回调  之前的调度方式是 操作系统 需要陷入到内核态 现在的调度是用户态
        EventContext write; // 写事件  协程  回调
        Event events = NONE;// 已经注册的事件
        MutexType mutex; 
    };

public:
    // 采用的epoll   -----> epoll_ctl epoll_create epoll_wait
    IOManager(size_t threads = -1,bool use_caller = true,const std::string& name = "");
    ~IOManager();

    int addEvent(int fd,Event event,std::function<void()> cb = nullptr);
    bool delEvent(int fd,Event event);
    bool cancelEvent(int fd,Event event);

    bool cancelAll(int fd);

    static IOManager *getThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    
    void contextResize(size_t size);
    //bool stopping(uint64_t& timer);

private:
    int m_epfd = 0;
    int m_tickleFds[2];

    std::atomic<size_t> m_pendingEventCount = {0};
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;

};

}

#endif