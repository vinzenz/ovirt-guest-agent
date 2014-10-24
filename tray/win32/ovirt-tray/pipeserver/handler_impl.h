#pragma once

#include "function_storage.h"

template< typename CallSignature >
struct FunctionHandlers {
    typedef CallSignature CallSignatureType;

    CallSignature Call;
    void(*Release)(FunctionStorage &);
    FunctionStorage(*Clone)(FunctionStorage const &);
};

template< typename T >
struct GenFun {
    typedef T(*Type)();
};

FunctionStorage SimpleClone(FunctionStorage const & s) {
    return s;
}

template< typename T >
FunctionStorage FunctorClone(FunctionStorage const & s) {
    FunctionStorage tmp = {};
    if (sizeof(T) <= sizeof(s.Data.Static)) {
        new (tmp.Data.Static) T(*reinterpret_cast<T const*>(s.Data.Static));
    }
    else {
        tmp.Data.Dynamic = new(std::nothrow) T(*reinterpret_cast<T const*>(s.Data.Dynamic));
    }
    return tmp;
}

void SimpleRelease(FunctionStorage&){}

template< typename T >
void FunctorRelease(FunctionStorage & s) {
    if (sizeof(T) <= sizeof(s.Data.Static)) {
        reinterpret_cast<T*>(s.Data.Static)->~T();
    }
    else {
        delete reinterpret_cast<T*>(s.Data.Dynamic);
    }
}

template< typename SignatureT >
struct CallerImpl;

template< typename HandlerT >
HandlerT FunctionHandler() {
    static HandlerT tmp = {
        &CallerImpl<typename HandlerT::CallSignatureType>::Function,
        &SimpleRelease,
        &SimpleClone
    };
    return tmp;
};

template< typename HandlerT, typename FunctorT >
HandlerT FunctorHandler() {
    static HandlerT tmp = {
        &CallerImpl<typename HandlerT::CallSignatureType>::Functor<FunctorT>,
        &FunctorRelease<FunctorT>,
        &FunctorClone<FunctorT>
    };
    return tmp;
};

template< typename HandlerT, typename ClassT, typename MethodT >
HandlerT MethodHandler() {
    static HandlerT tmp = {
        &CallerImpl<typename HandlerT::CallSignatureType>::Method<ClassT, MethodT>,
        &SimpleRelease,
        &SimpleClone
    };
    return tmp;
};

template< typename HandlerT >
struct MakeHandlerFun {
    template< typename ClassT, typename MethodT >
    static typename GenFun<HandlerT>::Type Get(ClassT * o, MethodT m)
    {
        // Method
        return MethodHandler<HandlerT, ClassT, MethodT>;
    }

    static typename GenFun<HandlerT>::Type Get(typename HandlerT::CallSignatureType f) {
        // Function
        return FunctionHandler<HandlerT>;
    }

    template< typename F >
    static typename GenFun<HandlerT>::Type Get(F f)
    {
        // Functor
        return FunctorHandler<HandlerT, F>;
    }
};

template<>
struct CallerImpl<void(*)(FunctionStorage&, DWORD, DWORD)> {
    static void Function(FunctionStorage&s, DWORD e, DWORD t) {
        reinterpret_cast<void(*)(DWORD, DWORD)>(s.Data.FunPtr)(e, t);
    }
    template< typename C, typename M>
    static void Method(FunctionStorage&s, DWORD e, DWORD t) {
        ((*reinterpret_cast<C*>(s.Data.MPtr.Object)).*reinterpret_cast<M>(s.Data.MPtr.Method))(e, t);
    }
    template< typename C >
    static void Functor(FunctionStorage&s, DWORD e, DWORD t) {
        if (sizeof(C) <= sizeof(s.Data.Static)) {
            (*reinterpret_cast<C*>(s.Data.Static))(e, t);
        }
        else {
            (*reinterpret_cast<C*>(s.Data.Dynamic))(e, t);
        }
    }
};

template<>
struct CallerImpl<void(*)(FunctionStorage&)> {
    static void Function(FunctionStorage&s) {
        reinterpret_cast<void(*)()>(s.Data.FunPtr)();
    }
    template< typename C, typename M>
    static void Method(FunctionStorage&s) {
        ((*reinterpret_cast<C*>(s.Data.MPtr.Object)).*reinterpret_cast<M>(s.Data.MPtr.Method))();
    }
    template< typename C >
    static void Functor(FunctionStorage&s) {
        if (sizeof(C) <= sizeof(s.Data.Static)) {
            (*reinterpret_cast<C*>(s.Data.Static))();
        }
        else {
            (*reinterpret_cast<C*>(s.Data.Dynamic))();
        }
    }
};
