#include "../vity/vity.h"

static vity::Logger::ptr g_logger = VITY_LOG_ROOT();

void test_fiber()
{
    VITY_LOG_INFO(g_logger) << "test in fiber start";

    // for(int i = 0; i < 10;i++)
    // {
    //     //vity::Fiber::YieldToReady();
    //     sleep(1);
    //     VITY_LOG_INFO(g_logger) << "test in fiber testing  " << std::to_string(i);
    // }
    
    static int s_count = 5;
    sleep(1);
    if(--s_count >= 0) {
        vity::Scheduler::GetThis()->schedule(&test_fiber, vity::GetThreadId());
    }

    VITY_LOG_INFO(g_logger) << "test in fiber over";
}

int main(int argc,char **argv)
{
    vity::Thread::SetName("main");
    VITY_LOG_INFO(g_logger) << "main";

    vity::Scheduler sc(3,true,"main");
    sc.start();
    
    VITY_LOG_INFO(g_logger) << "scheduler";
    sc.schedule(&test_fiber);

    sc.stop();
    VITY_LOG_INFO(g_logger) << "over";
    return 0;
}