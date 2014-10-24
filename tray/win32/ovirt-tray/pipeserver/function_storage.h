#pragma once


struct FunctionStorage {
    struct X {};
    union {
        struct {
            void * Object;
            int (X::*Method)();
        } MPtr;
        int(*FunPtr) ();
        char Static[32];
        void * Dynamic;
    } Data;
};


inline FunctionStorage MakeStorage(void(*f)()) {
    FunctionStorage s;
    s.Data.FunPtr = reinterpret_cast<int(*)()>(f);
    return s;
}

template< typename F >
inline FunctionStorage MakeStorage(F f) {
    FunctionStorage s;
    if (sizeof(f) <= sizeof(s.Data.Static)) {
        new (&s.Data.Static) F(f);
    }
    else {
        s.Data.Dynamic = new F(f);
    }
    return s;
}

template< typename ClassT, typename MethodT >
inline FunctionStorage MakeStorage(ClassT * o, MethodT m) {
    FunctionStorage s;
    s.Data.MPtr.Object = o;
    s.Data.MPtr.Method = reinterpret_cast<int(FunctionStorage::X::*)()>(m);
    return s;
}
