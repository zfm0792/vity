#ifndef __VITY_MACRO_H__
#define __VITY_MACRO_H__

#include <assert.h>
#include <string>
#include "utils.h"


#define VITY_ASSERT(x) \
    do{ \
        if(!(x)){ \
            VITY_LOG_ERROR(VITY_LOG_ROOT())<< "ASSERTION: " #x \
                << "\nbacktrace:\n" \
                << vity::BacktraceToString(100,2,"    "); \
            assert(x); \
        } \
    } while (0);
    
#define VITY_ASSERT2(x,w) \
    do{ \
        if(!(x)){ \
            VITY_LOG_ERROR(VITY_LOG_ROOT())<< "ASSERTION: " #x \
                << "\n" << w \
                << "\nbacktrace:\n" \
                << vity::BacktraceToString(100,2,"    "); \
            assert(x); \
        } \
    } while (0);
#endif