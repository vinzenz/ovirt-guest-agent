#pragma once

#include "critical_section.h"
#include "handler_functions.h"
#include "handle.h"

struct IOFunOverlapped : OVERLAPPED {
    int       References;
    IOHandler Callable;
    bool      Called;
    CriticalSection Sync;

    IOFunOverlapped(IOHandler callable)
        : OVERLAPPED()
        , References(1)
        , Callable(callable)
        , Called(false)
        , Sync()
    {
        hEvent = CreateEvent(0, TRUE, FALSE, 0);        
    }

    ~IOFunOverlapped() {        
        ::CloseHandle(hEvent);
    }
    
    void AddRef() {
        CriticalSection::Lock lock(Sync);
        References += 1;
    }
    
    void Release() {
        bool del = false;
        {
            CriticalSection::Lock lock(Sync);
            References -= 1;
            del = References < 1;
        }
        if(del) delete this;
    }

    void Perform(DWORD error, DWORD trans) {
        CriticalSection::Lock lock(Sync);
        if (!Called && Callable) {
            Called = true;
            Callable(error, trans);
        }
    }
};
