#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>


namespace vity{

static vity::Logger::ptr g_logger = VITY_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(Event event)
{
    switch(event){
        case IOManager::READ:
            return read; // EventContext 调度 协程 回调
        case IOManager::WRITE:
            return write;
        default:
            VITY_ASSERT2(false,"getContext");
    }
}
void IOManager::FdContext::resetContext(IOManager::FdContext::EventContext& ctx)
{
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}
// 如果触发了 就会去调度
void IOManager::FdContext::triggerEvent(Event event)
{
    VITY_ASSERT(events & event);
    events = (Event)(events & ~event);
    EventContext& ctx = getContext(event);

    if(ctx.cb){
        ctx.scheduler->schedule(&ctx.cb);
    }else{
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return ;
}
// 构造函数
// test_scheduler -----> Scheduler sc, sc.start
IOManager::IOManager(size_t threads,bool use_caller,const std::string& name)
    : Scheduler(threads,use_caller,name)
{
    m_epfd = epoll_create(1000);
    VITY_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    VITY_ASSERT(!rt);

    epoll_event event;
    memset(&event,0,sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0],F_SETFL,O_NONBLOCK);
    VITY_ASSERT(!rt);

    rt = epoll_ctl(m_epfd,EPOLL_CTL_ADD,m_tickleFds[0],&event);
    VITY_ASSERT(!rt);

    contextResize(32); // new

    start(); // 创建线程池 run起来   此时scheduler中是没有任何函数可被执行的
}

IOManager::~IOManager() // delete
{
    stop(); // 析构的时候主动调用stop()  此时所有的线程和协程都结束了
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i = 0; i < m_fdContexts.size();++i){
        if(m_fdContexts[i]){
            delete m_fdContexts[i];
        }
    }
}

int IOManager::addEvent(int fd,Event event,std::function<void()> cb)
{
    FdContext *fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() > fd){
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    }else{
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(fd_ctx->events & event){
        VITY_LOG_ERROR(g_logger) << "addEvent assert fd= "<< fd 
            << " event= " << event
            << "fd_ctx.event=" << fd_ctx->events;
        VITY_ASSERT(!(fd_ctx->events & event));
    }

    int op =  fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
        VITY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ","
            << op << "," << fd << "," << epevent.events << "):"
            << rt << "(" << errno << ") ("<< strerror(errno) << ")";
        return -1;
    }

    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    VITY_ASSERT(!event_ctx.scheduler
                && !event_ctx.fiber
                && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();
    if(cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        VITY_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
    }
    return 0;
}
bool IOManager::delEvent(int fd,Event event)
{
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd){
        return false;
    }

    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)){
        return false;
    }

    Event new_events = (Event)(fd_ctx->events &~ event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
        VITY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << "," << fd << "," << epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
    --m_pendingEventCount;
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}
bool IOManager::cancelEvent(int fd,Event event)
{
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)){
        return false;
    }

    Event new_events = (Event)(fd_ctx->events &~ event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        VITY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << "," << fd << "," << epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd)
{
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        VITY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << "," << fd << "," << epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    if(fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    VITY_ASSERT(fd_ctx->events == 0);
    return true;
}

IOManager* IOManager::getThis()
{
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle()
{
    if(hasIdleThreads()){
        return;
    }
    // 唤醒epoll_wait() ---- 触发m_tickleFds READ EVENT 
    // 所以会唤醒epoll_wait()  只是单纯为了唤醒 epoll_wait
    int rt = write(m_tickleFds[1],"a",1);
    VITY_ASSERT(rt == 1);
}

bool IOManager::stopping()
{
    return m_pendingEventCount == 0 && Scheduler::stopping();
}
// 空闲线程 协程执行的函数
// 空闲着的一定是没有任务需要去进行处理  陷入到epoll_wait等待中
// 当有事件来了  就会通过tickle 唤醒 就会去执行读或者写的事件回调
void IOManager::idle()
{
    epoll_event* events = new epoll_event[64]();
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
        delete[] ptr;
    });

    while(true) {
        if(stopping()) {
            VITY_LOG_INFO(g_logger) << "name=" << getName()
                                     << " idle stopping exit";
            break;
        }

        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 3000;
            rt = epoll_wait(m_epfd, events, 64, MAX_TIMEOUT);
            // 没有事件被触发
            if(rt < 0 && errno == EINTR) {
            } else {
                break; // rt > 0 ----- 有新的事件到来
            }
        } while(true);

        // 检查是哪个事件到了
        for(int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            // 如果是m_tickleFds 则需要将所有数据全部读完
            if(event.data.fd == m_tickleFds[0]) {
                uint8_t dummy;
                while(read(m_tickleFds[0], &dummy, 1) == 1);
                continue;
            }
            // 检查时怎样的事件被触发
            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= EPOLLIN | EPOLLOUT;
            }
            int real_events = NONE;
            if(event.events & EPOLLIN) {
                real_events |= READ;
            }
            if(event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            if((fd_ctx->events & real_events) == NONE) {
                continue;
            }

            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2) {
                VITY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << "," << fd_ctx->fd << "," << event.events << "):"
                    << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            // triggerEvent 调用触发的回调函数去执行
            if(real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        // 从idle里面切回去   表明idle已经执行完
        // 空闲的线程执行结束了 --- 新的请求已经被处理
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();
    }
}
void IOManager::contextResize(size_t size)
{
    m_fdContexts.resize(size);
    for(size_t i = 0; i < m_fdContexts.size();++i){
        if(!m_fdContexts[i]){
            m_fdContexts[i] = new FdContext();
            m_fdContexts[i]->fd = i;
        }
    }
}

}