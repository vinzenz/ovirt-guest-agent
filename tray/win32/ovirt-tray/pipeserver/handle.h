#pragma once

#include <windows.h>
#include <algorithm>

struct Handle {

    Handle(HANDLE h = INVALID_HANDLE_VALUE, bool auto_close = false)
    : handle_(h)
    , auto_close_(auto_close)
    {}

    ~Handle() {
        if (auto_close_) {
            Close();
        }
    }

    void Close() {
        if (IsValid()) {
            ::CloseHandle(handle_);
            handle_ = 0;
        }
    }

    bool IsValid() const {
        return handle_ != INVALID_HANDLE_VALUE
            && handle_ != 0;
    }

    operator bool() const {
        return IsValid();
    }

    operator HANDLE() const {
        return handle_;
    }

    void Swap(Handle & other) {
        std::swap(other.auto_close_, auto_close_);
        std::swap(other.handle_, handle_);
    }

protected:
    bool auto_close_;
    HANDLE handle_;
};

struct AutoHandle : Handle {
    AutoHandle(HANDLE h)
    : Handle(h, true)
    {}
};
