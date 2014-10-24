#pragma once

#include <windows.h>

struct CriticalSection : CRITICAL_SECTION {
    class Lock {
        CriticalSection * cs_;
    public:
        Lock(CriticalSection & cs)
            : cs_(&cs)
        {
            cs_->Accquire();
        }

        ~Lock() {
            Release();
        }

        void Release() {
            if (cs_) {
                cs_->Release();
            }
            cs_ = 0;
        }
    };

    CriticalSection()
        : CRITICAL_SECTION()
    {
        ::InitializeCriticalSection(this);
    }

    ~CriticalSection() {
        ::DeleteCriticalSection(this);
    }

    bool TryAccquire() {
        return ::TryEnterCriticalSection(this) == TRUE;
    }

    void Accquire() {
        ::EnterCriticalSection(this);
    }

    void Release() {
        ::LeaveCriticalSection(this);
    }
};
