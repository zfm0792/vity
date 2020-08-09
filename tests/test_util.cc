#include "../vity/vity.h"
#include <assert.h>

vity::Logger::ptr g_logger = VITY_LOG_ROOT();

void test_assert()
{

    VITY_LOG_INFO(g_logger) << vity::BacktraceToString(10, 2 ,"     ");
    //VITY_ASSERT(false)
    VITY_ASSERT2(0 == 1,"abcdef");
}

int main(int argc,char** argv)
{
    test_assert();
    return 0;
}