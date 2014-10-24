#pragma once

#include <string>
#include "stream_service.h"



struct NamedPipeService : StreamService {
    NamedPipeService(Service & service)
    : StreamService(service)
    {}

    void Close(Handle & h) {
        ::DisconnectNamedPipe(h);
        StreamService::Close(h);
    }

    DWORD Open(Handle & h, TCHAR const * path) {
        DWORD result = StreamService::Open(h, path);
        if (result == NO_ERROR) {
            DWORD mode = PIPE_READMODE_BYTE;
            if (!::SetNamedPipeHandleState(h, &mode, NULL, NULL)) {
                result = GetLastError();
                Close(h);
            }
        }
        return result;
    }
};

typedef BasicStream<NamedPipeService> NamedPipeStream;

struct NamedPipeAcceptor;
struct NamedPipeAcceptor {
    NamedPipeAcceptor(Service & service)
    : service_(service)
    , buffer_size_(0x10000)
    , name_()
    , attributes_(0)
    , stream_(service)
    {}

    ~NamedPipeAcceptor() {
        Close();
    }

    DWORD Open(std::basic_string<TCHAR> const & name, LPSECURITY_ATTRIBUTES attributes = 0) {
        Close();
        name_ = TEXT("\\\\.\\pipe\\") + name;
        attributes_ = attributes;
        return Create(stream_);
    }

    bool IsOpen() const {
        return !name_.empty();
    }

    void Close() {
        name_.clear();
        stream_.Close();
        attributes_ = 0;
    }

    DWORD AsyncAccept(NamedPipeStream & stream, IOHandler handler) {
        DWORD result = NO_ERROR;
        IOFunOverlapped * overlapped = new IOFunOverlapped(handler);
        
        if (!stream_.IsOpen()) {
            result = Create(stream_);
        }
        if (!result) {
            stream.Swap(stream_);
            overlapped->AddRef();
            BOOL connected = ::ConnectNamedPipe(stream.Native(), overlapped);
            if (!connected) {
                result = GetLastError();
                switch (result)
                {
                case ERROR_PIPE_CONNECTED:
                    overlapped->AddRef();
                    service_.PostCompletionStatus(NO_ERROR, 0, overlapped);
                    result = NO_ERROR;
                    break;                    
                case ERROR_IO_PENDING:                    
                    result = NO_ERROR;
                    break;
                default:
                    ::FlushFileBuffers(stream.Native());
                    ::DisconnectNamedPipe(stream.Native());                    
                case ERROR_NO_DATA:
                    if (stream.Native()) {
                        stream.Close();
                    }
                    service_.PostCompletionStatus(result, 0, overlapped);
                    break;
                }
            }
        }
        overlapped->Release();
        return result;
    }
protected:
    DWORD Create(NamedPipeStream & stream) {
        Handle h = ::CreateNamedPipe(
            name_.c_str(),
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            buffer_size_,
            buffer_size_,
            NMPWAIT_USE_DEFAULT_WAIT,
            attributes_
        );
        if (!h) {
            return GetLastError();
        }
        if (!service_.Register(h)) {
            h.Close();
            return GetLastError();
        }
        stream.Assign(h);
        return ERROR_SUCCESS;
    }
private:
    NamedPipeAcceptor(NamedPipeAcceptor const &);
    NamedPipeAcceptor & operator=(NamedPipeAcceptor const &);
protected:
    Service & service_;
    DWORD buffer_size_;
    std::basic_string<TCHAR> name_;
    LPSECURITY_ATTRIBUTES attributes_;
    NamedPipeStream stream_;
};

