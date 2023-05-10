#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    extern __thread int t_cachedTid;
    void cachTid();
    
    inline int tid()
    {
        if(__builtin_expect(t_cachedTid == 0, 0))
        {
            cachTid();
        }
        return t_cachedTid;
    }
}