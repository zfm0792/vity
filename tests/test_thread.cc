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

    YAML::Node root = YAML::LoadFile("/home/fredzhan/vity/bin/conf/log2.yml");
    vity::Config::LoadFromYaml(root);
    std::vector<vity::Thread::ptr> thrs;

    for(int i = 0; i < 2; i++){
        
        vity::Thread::ptr thr(new vity::Thread(&fun2,"name_"+std::to_string(i*2)));
        vity::Thread::ptr thr2(new vity::Thread(&fun3,"name_"+std::to_string(i*2+1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }
    for(size_t i = 0; i < thrs.size(); i++){
        thrs[i]->join();
    }
    VITY_LOG_INFO(g_logger) << "thread test end";
    return 0;
}