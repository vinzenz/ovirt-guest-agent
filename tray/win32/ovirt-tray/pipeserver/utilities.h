#pragma once

#include <windows.h>

inline DWORD GetPlatformThreadCount() {
    SYSTEM_INFO info = {};
    ::GetNativeSystemInfo(&info);
    return info.dwNumberOfProcessors;
}
