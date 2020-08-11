#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>
#include "../vity/vity.h"

void test_ucontext()
{
/*
typedef struct ucontext_t
{
unsigned long int __ctx(uc_flags); 
struct ucontext_t *uc_link; // 运行终止时系统会恢复uc_link指向的上下文
stack_t uc_stack;   // 上下文使用的栈
mcontext_t uc_mcontext; // 保存上下文的特定机器表示 调用线程的特定寄存器
sigset_t uc_sigmask;    //上下文中阻塞信号集合
struct _libc_fpstate __fpregs_mem;
} ucontext_t;
*/
    ucontext_t context;

    getcontext(&context); // 初始化ucp结构体 将当前的上下文保存到ucp中

    puts("hello world");
    sleep(1);
    setcontext(&context); // 设置当前上下文的ucp  应该由getcontext 或者 makecontext取得
    // 修改通过getcontext取得的上下文ucp 必须先调用getcontext
    // 然后给上下文指定一个栈空间ucp->stack 设置后继ucp->uc_link

    //void makecontext(ucontext_t *ucp, void (*func)(), int argc, ...);

    // int swapcontext(ucontext_t *oucp, ucontext_t *ucp); 激活 执行func函数


}


ucontext_t child1,main1;
void fun1(void* arg)
{

    puts("1");
    puts("11");
    puts("111");
    puts("1111");
    swapcontext(&child1,&main1);
    puts("11111");
    puts("111111");
    swapcontext(&child1,&main1);
}

void context_test()
{
    void *stack = malloc(1024*1024);
    if(stack == nullptr)
    {
        return;
    }
    getcontext(&main1); // 获取上下文

    getcontext(&child1); // 获取上下文
    child1.uc_stack.ss_sp = stack; // 设置栈帧起始
    child1.uc_stack.ss_size = 1024*1024; // 设置栈大小
    child1.uc_stack.ss_flags = 0;
    child1.uc_link = nullptr; // 设置后继上下文

    makecontext(&child1,(void(*)(void))fun1,0); // 修改上下文指向

    swapcontext(&main1,&child1);// 切换到child1上下文 保存当前上下文到main1
    puts("main");
}

vity::Logger::ptr g_logger = VITY_LOG_ROOT();

void run_in_fiber()
{
    VITY_LOG_INFO(g_logger) << "run_in_fiber begin";
    vity::Fiber::YieldToHold();
    VITY_LOG_INFO(g_logger) << "run_in_fuiber hold";
    vity::Fiber::YieldToHold();
    VITY_LOG_INFO(g_logger) << "run_in_fiber end";
}

void test_fiber()
{
    VITY_LOG_INFO(g_logger) << "main begin -1";
    {
        // getcontext
        // makecontext ----- 子协程
        // swapcontext
        // 主协程的上下文环境被记录下来
        // 此时还没有swapcontext
        vity::Fiber::GetThis();
        VITY_LOG_INFO(g_logger) << "main begin";
        vity::Fiber::ptr fiber(new vity::Fiber(run_in_fiber));
        fiber->call();
        VITY_LOG_INFO(g_logger) << "main after swapIn";
        fiber->call();
        VITY_LOG_INFO(g_logger) << "main after end";
        fiber->call();
    }
    VITY_LOG_INFO(g_logger) << "main end -1";
}
int main(int argc,const char* argv[])
{
    //test_ucontext();
    // context_test();
    // puts("333333333333333");
    // swapcontext(&main1,&child1);
    // puts("333333333333333");

    test_fiber();
    return 0;
}