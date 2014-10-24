#pragma once

#include <windows.h>
#include <process.h>

#include <vector>
#include "utilities.h"
#include "iofunoverlapped.h"

struct CompletionWrapper {
    CompletionWrapper(CompletionHandler callable)
    : Callable(callable)
    {}

    void operator()(DWORD, DWORD) {
        if(Callable) Callable();
    }

    CompletionHandler Callable;
};

enum {
     OVERLAPPED_CONTAINS_RESULT = 1,
};

struct Service {

    Service()
    : port_(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0))
    {
    }

    ~Service()
    {
        Stop();
    }

    void Start() {
        threads_.push_back((HANDLE) _beginthread(&Service::Run_, 0, this));
    }

    void Run() {
        threads_.push_back(GetCurrentThread());        
        RunInternal();
    }

    void Stop() {
        if (!threads_.empty())  {
            for (std::size_t i = 0; i < threads_.size(); ++i) {
                ::PostQueuedCompletionStatus(port_, 0, 0, 0);
            }
            ::WaitForMultipleObjects(DWORD(threads_.size()), &threads_[0], TRUE, INFINITE);
            threads_.clear();
        }
    }

    DWORD PostCompletionStatus(DWORD err, DWORD trans, OVERLAPPED * overlapped) {
        if (!::PostQueuedCompletionStatus(port_, err, trans, overlapped)) {
            return GetLastError();
        }
        return NO_ERROR;
    }    

    DWORD Post(CompletionHandler f) {
        if (::PostQueuedCompletionStatus(port_, 0, 0, new IOFunOverlapped(CompletionWrapper(f)))) {
            return NO_ERROR;
        }
        return GetLastError();
    }

    bool Register(Handle & h) {
        return ::CreateIoCompletionPort(h, port_, 0, 0) == port_;
    }
protected:
    void RunInternal() {
        DWORD transferred = 0;
        ULONG_PTR key = 0;
        LPOVERLAPPED overlapped = 0;
        for(;;) {
            BOOL getResult = ::GetQueuedCompletionStatus(port_, &transferred, &key, &overlapped, INFINITE);
            if (getResult) {
                if (key == 0 && transferred == 0 && overlapped == 0) {
                    break;
                }
            }
            if (overlapped) {
                DWORD last_error = GetLastError();
                if (!getResult) {
                    overlapped->Internal = last_error;
                    overlapped->InternalHigh = transferred;
                }
                else {
                    last_error = NO_ERROR;
                }
                reinterpret_cast<IOFunOverlapped*>(overlapped)->Perform(last_error, transferred);
                reinterpret_cast<IOFunOverlapped*>(overlapped)->Release();
            }
        }
    }

    static void Run_(void * p) {
        reinterpret_cast<Service*>(p)->RunInternal();
    }
private:
    Service(Service const &);
    Service & operator=(Service const &);
private:
    std::vector<HANDLE> threads_;
    Handle port_;
};