#include "utils.h"

namespace vity {

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint32_t GetFiberId() {
    return 0;
}

std::string GetThreadName()
{
    return std::string("");
}

}