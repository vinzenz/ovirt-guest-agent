#pragma once

template <class T>
class ref_ptr {
public:
    typedef T type;

    template< typename U >
    explicit ref_ptr(U * u)
    : ptr_(u)
    , refs_(new long(1))
    {}
    
    explicit ref_ptr(T * u)
    : ptr_(u)
    , refs_(new long(1))
    {}
    
    ref_ptr()
    : ptr_()
    , refs_()
    {}

    template< typename U >
    ref_ptr(ref_ptr<U> const & other)
    : ptr_(other.ptr_)
    , refs_(other.refs_)
    {
        ++*refs_;
    }


    explicit ref_ptr(ref_ptr const & p) throw()
        : ptr_(p.ptr_)
        , refs_(p.refs_)
    {
        ++*refs_;
    }

    ~ref_ptr () {
        release();
    }

    void swap(ref_ptr & other) {
        std::swap(other.ptr_, ptr_);
        std::swap(other.refs_, refs_);
    }

    ref_ptr & operator= (ref_ptr p) {
        swap(p);
        return *this;
    }
    
    ref_ptr & operator=(T * p) {
        ref_ptr tmp(p);
        swap(tmp);
        return *this;
    }
    
    template< typename U >
    ref_ptr & operator=(U * p) {
        ref_ptr tmp(p);
        swap(tmp);
        return *this;
    }

    template< typename U >
    ref_ptr & operator=(ref_ptr<U> const & p) {
        ref_ptr<T> tmp = p;
        swap(tmp);
        return *this;
    }
        
    T * get() const {
        return ptr_;
    }
    
    template< typename U >
    U * dynamic_as() const {
        return dynamic_cast<U*>(get())
    }
    
    template< typename U >
    U * static_as() const {
        return static_cast<U*>(get())
    }

    T& operator*() const {
        return *get();
    }
    T* operator->() const {
        return get();
    }

private:
    void release() {
        if (--*refs_ == 0) {
            delete refs_;
            delete ptr_;
        }
    }

protected:
    T* ptr_;
    long * refs_;

};
