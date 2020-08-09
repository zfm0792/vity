#include "../vity/vity.h"

vity::Logger::ptr g_logger = VITY_LOG_ROOT();

void func1()
{
    VITY_LOG_INFO(g_logger) << " name: "<< vity::Thread::GetName()
                            << " this.name: "<< vity::Thread::GetThis()->getName()
                            << " id: "<< vity::GetThreadId()
                            << " this.id "<< vity::Thread::GetThis()->getId();
    sleep(60);
}
int count = 0;
//vity::Semphore sem(2);
//vity::NullMutex s_mutex;
vity::RWMutex s_rwmutex;
void func2()
{
    for(int i = 0; i < 10000000;i++){
        vity::RWMutex::ReadLock lock(s_rwmutex);
        //sem.wait();
        count++;
        //sem.notify();
    }
}

int main()
{
    VITY_LOG_INFO(g_logger) << "thread test start";
    // vity::Thread::SetName("main thread");

    std::vector<vity::Thread::ptr> thrs;

    for(int i = 0; i < 5; i++){
        vity::Thread::ptr thr(new vity::Thread(&func2,"name_"+std::to_string(i)));
        thrs.push_back(thr);
    }
    for(int i = 0; i < 5; i++){
        thrs[i]->join();
    }
    VITY_LOG_INFO(g_logger) << "thread test end";
    VITY_LOG_INFO(g_logger) << " count= "<< count;
    return 0;
}