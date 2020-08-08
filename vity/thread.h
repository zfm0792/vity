#ifndef __WRI_THREAD_H__
#define __WRI_THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>

namespace WRI{

class Thread{
public:
    typedef std::shared_ptr<Thread>tr;
    Thread(std::function<void()> cb,const std::string& name);
    ~Thread();

    pid_t getId() const {return m_id;}
    const std::string& getName() const {return m_name;}

    void join();

    static Thread* GetThis();
    static const std::string& GetName();
    static void setName(const std::string& name);// 主线程不是我们自己产生
private:
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;
    
    static void *run(void *arg);
private:
    pid_t m_id;
    pthread_t m_thread;
    std::function<void()> m_cb;
    std::string m_name;
};

}


#endif