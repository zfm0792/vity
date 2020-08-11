#include "utils.h"
#include <execinfo.h>

#include "log.h"

namespace vity {

vity::Logger::ptr g_logger = VITY_LOG_NAME("system");

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint32_t GetFiberId() {
    return 0;
}

void Backtrace(std::vector<std::string>& bt,int size,int skip)
{
    void **array = (void**)malloc(sizeof(void*) * size);
    if(array == NULL){
        VITY_LOG_ERROR(g_logger) << "Backtrace malloc array error";
        return;
    }
    int nptrs = ::backtrace(array,size);
    char** strings = backtrace_symbols(array,nptrs);

    if(strings == NULL){
        VITY_LOG_ERROR(g_logger) << "backtrace_symbols error";
        free(array);
        return;
    }

    for(int i = skip;i < nptrs;++i){
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
    return ;
}
std::string BacktraceToString(int size,int skip,std::string prefix)
{
    std::vector<std::string> bt;
    Backtrace(bt,size,skip);
    
    std::stringstream ss;
    
    for(size_t i = 0; i < bt.size(); i++){
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}

}