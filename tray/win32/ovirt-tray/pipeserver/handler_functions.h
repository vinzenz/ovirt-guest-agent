#pragma once

#include "handler_impl.h"

struct IOHandler {
    IOHandler()
    : storage_()
    , handler_(0)
    {}

    IOHandler(IOHandler const & other)
    : storage_(other ? other.handler_().Clone(other.storage_) : FunctionStorage())
    , handler_(other.handler_)
    {}

    template< typename F >
    IOHandler(F f)
    : storage_(MakeStorage(f))
    , handler_(MakeHandlerFun<HandlerT>::Get(f))
    {}

    template< typename ClassT, typename MethodT >
    IOHandler(ClassT * o, MethodT * m)
    : storage_(MakeStorage(o, m))
    , handler_(MakeHandlerFun<HandlerT>::Get(o, m))
    {}

    ~IOHandler() {
        if (handler_) {
            handler_().Release(storage_);
        }
    }

    operator bool() const {
        return handler_ != 0;
    }

    void operator()(DWORD err, DWORD trans){
        handler_().Call(storage_, err, trans);
    }
protected:
    typedef FunctionHandlers<void(*)(FunctionStorage&, DWORD, DWORD)> HandlerT;
    FunctionStorage storage_;
    HandlerT (*handler_)();
};

struct CompletionHandler {
    CompletionHandler()
    : storage_()
    , handler_(0)
    {}

    CompletionHandler(CompletionHandler const & other)
        : storage_(other ? other.handler_().Clone(other.storage_) : FunctionStorage())
        , handler_(other.handler_)
    {}

    template< typename F >
    CompletionHandler(F f)
        : storage_(MakeStorage(f))
        , handler_(MakeHandlerFun<HandlerT>::Get(f))
    {}

    template< typename ClassT, typename MethodT >
    CompletionHandler(ClassT * o, MethodT m)
        : storage_(MakeStorage(o, m))
        , handler_(MakeHandlerFun<HandlerT>::Get(o, m))
    {}

    ~CompletionHandler() {
        if (handler_) {
            handler_().Release(storage_);
        }
    }

    operator bool() const {
        return handler_ != 0;
    }

    void operator()(){
        handler_().Call(storage_);
    }
protected:
    typedef FunctionHandlers<void(*)(FunctionStorage&)> HandlerT;
    FunctionStorage storage_;
    HandlerT(*handler_)();
};