#include "thread.h"

namespace WRI{
    static thread_local Thread* t_thread = nullptr;
    static thread_local std::string t_thread_name = "UNKNOW";
    
    Thread* Thread::GetThis(){
        return t_thread;
    }
    const std::string& Thread::GetName(){
        return t_thread_name;
    }

    void Thread::setName(const std::string& name)
    {
        if(t_thread){
            t_thread->m_name = name;
        }
        t_thread_name = name;
    }


    Thread::Thread(std::function<void()> cb,const std::string& name)
    {
        if(name.empty()){
            m_name = "UNKNOW";
        }
        int rt = pthread_create(&m_thread,nullptr,&Thread::run,this);
        if(rt){
            // log
        }
    }
    Thread::~Thread()
    {
        if(m_thread)
            pthread_detach(m_thread);
    }
    void *Thread::run(void *arg)
    {
        Thread* thread = (Thread*)arg;
        t_thread = thread;
        thread->m_id = 
    }

    tid_t getId() const {return m_id;}
    const std::string& getName() const {return m_name;}

    void Thread::join()
    {
        if(m_thread){
            int rt = pthread_join(m_thread,nullptr);
            if(rt){
                // log
            }
            m_thread = 0;
        }
    }


}