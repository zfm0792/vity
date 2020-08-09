#ifndef __VITY_UTIL_H__
#define __VITY_UTIL_H__

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace vity{

    pid_t GetThreadId();
    uint32_t GetFiberId();
    std::string GetThreadName();

    void Backtrace(std::vector<std::string>& bt,int size,int skip = 1);
    std::string BacktraceToString(int size,int skip = 2,std::string prefix = "");
}

#endif