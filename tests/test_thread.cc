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
vity::Semphore sem;
//vity::NullMutex s_mutex;
vity::RWMutex s_rwmutex;
void func2()
{
    sem.wait();
    VITY_LOG_INFO(g_logger) << "child thread test 2";
    for(int i = 0; i < 10000;i++){
        //vity::RWMutex::ReadLock lock(s_rwmutex);
        count++;
    }
    VITY_LOG_INFO(g_logger) << "child thread test 3";
}

void fun2() {
    while(true) {
        VITY_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3() {
    while(true) {
        VITY_LOG_INFO(g_logger) << "========================================";
    }
}

int main()
{
    VITY_LOG_INFO(g_logger) << "thread test start";
    vity::Thread::SetName("main thread");

    // YAML::Node root = YAML::LoadFile("/home/fredzhan/vity/bin/conf/log2.yml");
    // vity::Config::LoadFromYaml(root);
    std::vector<vity::Thread::ptr> thrs;

        
    vity::Thread::ptr thr(new vity::Thread(&func2,"name_"+std::to_string(1)));


    VITY_LOG_INFO(g_logger) << "main thread test ";
    sleep(2);
    VITY_LOG_INFO(g_logger) << "main thread test ";
    sem.notify();
    VITY_LOG_INFO(g_logger) << "main thread test 2";
    thr->join();
    VITY_LOG_INFO(g_logger) << "count="<<count;
    VITY_LOG_INFO(g_logger) << "thread test end";
    return 0;
}