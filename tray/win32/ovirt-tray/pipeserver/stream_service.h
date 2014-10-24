#pragma once

#include "service.h"
#include "iofunoverlapped.h"


struct StreamService {
    StreamService(Service & s)
    : service_(s)
    {}

    bool Register(Handle & h) {
        return service_.Register(h);
    }

    DWORD Open(Handle & h, TCHAR const * path) {
        h = ::CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
        if (!h) {
            return GetLastError();
        }
        if (!service_.Register(h)) {
            return GetLastError();
        }
        return NO_ERROR;
    }

    void Close(Handle & h) {
        h.Close();        
    }

    void Cancel(Handle & h) {
        ::CancelIoEx(h, 0);
    }

    DWORD AsyncRead(Handle & h, BYTE * buffer, DWORD buffer_size, IOHandler completion) {        

        IOFunOverlapped * trans = new IOFunOverlapped(completion);
        DWORD transferred = 0;
        trans->AddRef();
        if (!::ReadFile(h, buffer, buffer_size, &transferred, trans)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                // On an error we have to release both references
                trans->Release();
                trans->Release();
                return GetLastError();
            }
        }
        else {
            trans->AddRef();
            service_.PostCompletionStatus(NO_ERROR, transferred, trans);
        }
        trans->Release();
        return ERROR_SUCCESS;
    }

    DWORD AsyncWrite(Handle & h, BYTE const * buffer, DWORD buffer_size, IOHandler completion) {
        IOFunOverlapped * trans = new IOFunOverlapped(completion);
        DWORD transferred = 0;
        trans->AddRef();
        if (!::WriteFile(h, buffer, buffer_size, &transferred, trans)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                // On an error we have to release both references
                trans->Release();
                trans->Release();
                return GetLastError();
            }
        }
        else {
            trans->AddRef();
            service_.PostCompletionStatus(NO_ERROR, transferred, trans);
        }
        trans->Release();
        return ERROR_SUCCESS;
    }    

protected:
    Service & service_;
};

template< typename StreamServiceT = StreamService >
struct BasicStream {
    BasicStream(Service & s)
    : handle_(INVALID_HANDLE_VALUE)
    , service_(s)
    , stream_service_(s)
    {}    

    void Swap(BasicStream & other) {
        std::swap(handle_, other.handle_);
    }

    DWORD Open(TCHAR const * path) {
        Close();
        return stream_service_.Open(Native(), path);
    }

    void Assign(Handle & h) {
        Close();
        handle_ = h;
    }

    Handle & Native() {
        return handle_;
    }

    void Cancel() {
        stream_service_.Cancel(handle_);
    }

    void Close() {
        stream_service_.Close(handle_);
    }

    bool IsOpen() const {
        return handle_;
    }

    Service & GetService() {
        return service_;
    }

    DWORD AsyncRead(BYTE * buffer, DWORD buffer_size, IOHandler completion) {
        return stream_service_.AsyncRead(handle_, buffer, buffer_size, completion);
    }

    DWORD AsyncWrite(BYTE const * buffer, DWORD buffer_size, IOHandler completion) {
        return stream_service_.AsyncWrite(handle_, buffer, buffer_size, completion);
    }

protected:
    Handle handle_;
    Service & service_;
    StreamServiceT stream_service_;
};

typedef BasicStream<> Stream;
