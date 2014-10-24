#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cassert>
#include "ref_ptr.h"

struct null_t{};
namespace detail {
    typedef unsigned char uint8_t;
    typedef INT32 int32_t;
    typedef INT64 int64_t;
    

    /* Basic types: All values are little endian encoded
     * <uint8>      1 byte  (8-bit unsigned integer, two's complement)
     * <int32>      4 bytes (32-bit signed integer, two's complement)
     * <int64>      8 bytes (64-bit signed integer, two's complement)
     * <double>     8 bytes (64-bit IEEE 754 floating point)
     * 
     * document     ::= size value
     * value:       ::= 
     *                byte
     *              | bool
     *              | int32
     *              | int64
     *              | double
     *              | string
     *              | list
     *              | object
     *              | map
     *              | binary
     *              | datetime
     *              | null
     * null         ::= '\x00'                          # 
     * byte         ::= '\x01' <uint8>                  # 
     * bool         ::= '\x02' ('\x00' | '\x01')        # '\x00' when false, '\x01' when true
     * int32        ::= '\x03' <int32>                  # 
     * int64        ::= '\x04' <int64>                  # 
     * double       ::= '\x05' <double>                 # 
     * string       ::= '\x06' size cstring             # size must match the number of elements in cstring including 0 termination
     * list         ::= '\x07' size value*              # size must match the number of value elements
     * object       ::= '\x08' size (string value)*     # size must match the number of (string value) tuples
     * map          ::= '\x09' size (value value)*      # size must match the number of (value value) tuples
     * binary       ::= '\x0A' size <uint8>*            # size must match count of <uint8> elements
     * datetime     ::= '\x0B' size int64_t             # seconds since UTC epoch
     * cstring      ::= <uint8>* '\x00'                 # 0-terminated utf-8 encoded string
     * size         ::= <int64>
     */
    
    enum data_type {
        kDataType_First     = 0x00,
        kDataType_Null      = 0x00,
        kDataType_Byte      = 0x01,
        kDataType_Bool      = 0x02,
        kDataType_Int32     = 0x03,
        kDataType_Int64     = 0x04,
        kDataType_Double    = 0x05,
        kDataType_String    = 0x06,
        kDataType_List      = 0x07,
        kDataType_Object    = 0x08,
        kDataType_Map       = 0x09,
        kDataType_Binary    = 0x0A,
        kDataType_DateTime  = 0x0B,
        
        // Leave at end
        kDataType_Count,
        kDataType_Last      = kDataType_Count - 1,
        kDataType_Invalid   = -1
    };       
    
    template< typename T >
    void push(std::vector<uint8_t> & target, T v) {
        uint8_t const * ptr = reinterpret_cast<uint8_t const *>(&v);
        target.insert(target.end(), ptr, ptr + sizeof(v));
    }

    template< typename T >
    void push(std::vector<uint8_t> & target, T const * tptr, size_t elements) {
        uint8_t const * ptr = reinterpret_cast<uint8_t const *>(ptr);
        target.insert(target.end(), ptr, ptr + (sizeof(T) * elements));
    }
    
    template< typename Element, size_t SizeDiff >        
    class data_view {
    public:
        typedef Element value_type;
        typedef value_type const * iterator;
        typedef value_type const * const_iterator;
        typedef value_type const * const_pointer;
        typedef size_t size_type;
        static Element const EMPTY = 0;
     
        data_view()
        : p_(&EMPTY)
        , length_(0)
        {}
     
        data_view(const_pointer b, const_pointer e) 
        : p_(b)
        , length_(size_type(e - b) >= SizeDiff ? size_type(e - b) - SizeDiff : 0)
        {
            assert(b <= e);
        }
        
        data_view(data_view const & that)
        : p_(that.p_)
        , length_(that.length_)
        {}

        value_type operator[](size_type index) const {
            assert(index < length_);
            return p_[index];
        }        

        const_iterator begin() const {
            return p_;
        }

        const_iterator end() const {
            return p_ + length_;
        }

        size_type size() const {
            return length_;
        }

        const_pointer data() const {
            return p_;
        }
        
        size_t diff() const {
            return SizeDiff;
        }
    protected:
        const_pointer p_;
        size_type  length_;
    };
    
    typedef data_view<uint8_t, 0> binary_view;

    class string_view : public data_view<char, 1> {
    public:
        string_view()
        : data_view()
        {}
    
        string_view(const_pointer b, const_pointer e)
        : data_view(b, e)
        {}
        
        const_pointer c_str() const {
            return data();
        }
    };        
    
    class value {
    protected:
        // Only inheriting classes are 
        // allowed to set the type
        value(data_type type)
        : type_(type)
        {}
    public:
        value()
        : type_(kDataType_Invalid)
        {}
    
        data_type type() const {
            return type_;
        }
        
        bool valid() const {
            return type_ != kDataType_Invalid
                && type_ >= kDataType_First 
                && type_ <= kDataType_Last
                ;
        }
        
        bool operator==(value const & rhs) const {
            return equals(rhs);
        }
        
        virtual void append(std::vector<uint8_t> & target) const = 0;        
        virtual bool equals(value const & rhs) const = 0;
        virtual size_t hash() const = 0;
    protected:
        data_type const type_;
    };
    
    typedef ref_ptr<value> value_ptr;

    bool consume(uint8_t const *& b, uint8_t const * e, uint8_t & target) {
        return false;
    }
    bool consume(uint8_t const *& b, uint8_t const * e, int32_t & target) {
        return false;
    }
    bool consume(uint8_t const *& b, uint8_t const * e, int64_t & target) {
        return false;
    }
    bool consume(uint8_t const *& b, uint8_t const * e, double & target) {
        return false;
    }
    bool consume(uint8_t const *& b, uint8_t const * e, value_ptr & target);
    
    template< typename T >    
    bool consume(uint8_t const *& b, uint8_t const * e, ref_ptr<T> & target) {
        return T::consume(b, e, target);
    }
    
    class null_value : public value {
    public:
        null_value()
        : value(kDataType_Null)
        {}
        
        null_value(null_t const &)
        : value(kDataType_Null)
        {}
        
        static bool consume(uint8_t const *& b,
                            uint8_t const * e,
                            ref_ptr<null_value> & target);
        void append(std::vector<uint8_t> & target) const {
            push<uint8_t>(target, type());
        }
        bool equals(value const & rhs) const {
            return true;
        }
        size_t hash() const { return 0; }
    };
    
    template< typename PODType, data_type DataType >
    class basic_pod_value : public value {        
    public:
        typedef value base_type;
        basic_pod_value(PODType v = PODType())
        : base_type(DataType)
        , data_(v)
        {}                

        PODType value() const {
            return data_;
        }                
        
        void value(PODType v) {
            data_ = v;
        }

        static bool consume(uint8_t const *& b,
                            uint8_t const * e,
                            ref_ptr<basic_pod_value> & target);
        void append(std::vector<uint8_t> & target) const {
            push<uint8_t>(target, DataType);
            push<PODType>(target, data_);
        }
        bool equals(base_type const & rhs) const {
            return rhs.type() == type()
                && data_ == static_cast<basic_pod_value const &>(rhs).data_;
        }
        size_t hash() const {
            return std::tr1::hash<PODType>()(data_);
        }
    protected:
        PODType data_;
    };
    
    typedef basic_pod_value<double, kDataType_Double>       double_value;
    typedef basic_pod_value<uint8_t, kDataType_Byte>        byte_value;
    typedef basic_pod_value<int32_t, kDataType_Int32>       int32_value;
    typedef basic_pod_value<int64_t, kDataType_Int64>       int64_value;
    typedef basic_pod_value<int64_t, kDataType_DateTime>    datetime_value;
    
    template< typename ViewType, data_type DataType >
    class basic_view_value : public value {        
    public:
        typedef value base_type;
        basic_view_value(ViewType view)
        : base_type(DataType)
        , data_(view)
        {}                

        ViewType value() const {
            return data_;
        }                

        void value(ViewType v) {
            data_ = v;
        }

        static bool consume(uint8_t const *& b,
                            uint8_t const * e,
                            ref_ptr<basic_view_value> & target)
        {
            uint8_t const * const backup = b;
            bool result = false;
            if(b < e) {
                uint8_t t = kDataType_Invalid;
                result = detail::consume(b, e, t) && t == DataType;
                int64_t count = 0;
                result = result && detail::consume(b, e, count);
                result = (e - b) <= count;
                if(result) {
                    typedef ViewType::const_pointer ptr_t;
                    target = new basic_view_value(ViewType(reinterpret_cast<ptr_t>(b), reinterpret_cast<ptr_t>(b) + count));
                }
            }
            if(!result) {
                b = backup;
            }
            return result;
        }
        void append(std::vector<uint8_t> & target) const {
            push<uint8_t>(target, type());
            push<int64_t>(target, data_.size() + data_.diff());
            if(data_.size() > 0) {
                push(target, data_.data(), data_.size() + data_.diff());
            }
        }
        
        bool equals(base_type const & rhs) const {            
            return type() == rhs.type()
                && static_cast<basic_view_value const &>(rhs).value().data() == value().data();
        }
        
        size_t hash() const {
            return std::tr1::hash<typename ViewType::const_pointer>()(data_.data());
        }
    protected:
        ViewType data_;
    };
        
    typedef basic_view_value<string_view, kDataType_String> string_value;
    typedef basic_view_value<binary_view, kDataType_Binary> binary_value;
    
    class list_value : public value {
    public:
        typedef value base_type;
    
        list_value(std::vector<value_ptr> const & v)
        : base_type(kDataType_List)
        , data_(new std::vector<value_ptr>(v))
        {}
        
        list_value(ref_ptr<std::vector<value_ptr> > const & v)
        : base_type(kDataType_List)
        , data_(v)
        {}

        list_value()
        : base_type(kDataType_List)
        , data_()
        {}
        
        ref_ptr<std::vector<value_ptr> > value() const {
            return data_;        
        }    
        
        void value(std::vector<value_ptr> const & v) {
            data_ = new std::vector<value_ptr>(v);
        }
        
        static bool consume(uint8_t const *& b,
                            uint8_t const * e,
                            ref_ptr<list_value> & target)
        {
            ref_ptr<list_value> list;
            bool result = false;
            uint8_t const * backup = b;
            if(b < e) {
                uint8_t type8 = kDataType_Invalid;
                result = detail::consume(b, e, type8);
                int64_t count = 0;
                result = result && detail::consume(b, e, count);
                list = new list_value();
                list->data_->resize(size_t(count));
                for(int64_t i = 0; result && i < count; ++i) {
                    result = detail::consume(b, e, (*list->data_)[i]);
                }
            }
            if(!result) {
                b = backup;
            }
            else {
                target = list;
            }
            return result;
        }
                            
        void append(std::vector<uint8_t> & target) const {
            push<uint8_t>(target, type());
            push<int64_t>(target, data_->size());            
            for(std::vector<value_ptr>::const_iterator it = data_->begin(), end = data_->end(); it != end; ++it) {
                (*it)->append(target);
            }
        }

        bool equals(base_type const & rhs) const {            
            return rhs.type() == type()
                && data_.get() != 0 
                && static_cast<list_value const &>(rhs).data_.get() != 0 
                && (data_.get()) == static_cast<list_value const &>(rhs).data_.get();
        }

        size_t hash() const {
            return size_t(data_.get());
        }
    protected:
        ref_ptr<std::vector<value_ptr> > data_;
    };

    struct value_hasher {
        size_t operator()(value_ptr const & v) const {
            size_t result = 0;
            if(v.get() && v->valid()) {
                result = v->hash();
            }
            return result;
        }     
    };
    
    struct value_equals
    : public std::binary_function<value_ptr, value_ptr, bool> {
        bool operator()(value_ptr const & lhs, value_ptr const & rhs) const {
            bool result = false;
            if((lhs.get() != 0) == (rhs.get() != 0)) {
                if(lhs.get() != 0 && rhs.get() != 0) {
                    if(lhs->type() == rhs->type()) {
                        return *lhs == *rhs;
                    }
                }
            }
            return result;
        };    
    };

    template< typename Key >
    class mapped_value : public value {
    public:
        typedef Key key_type;
        typedef value_ptr value_type;
        typedef std::tr1::unordered_map<key_type, value_type, value_hasher, value_equals> map_type;
        typedef value base_type;

        mapped_value(map_type const & v)
        : base_type(kDataType_List)
        , data_(new map_type(v))
        {}

        mapped_value(ref_ptr<map_type> const & v)
        : base_type(kDataType_List)
        , data_(v)
        {}

        mapped_value()
        : base_type(kDataType_List)
        , data_()
        {}

        ref_ptr<map_type> value() const {
            return data_;        
        }    

        void value(map_type const & v) {
            data_ = new map_type(v);
        }

        static bool consume(uint8_t const *& b,
                            uint8_t const * e,
                            ref_ptr<mapped_value> & target)
        {
            ref_ptr<mapped_value> map;
            bool result = false;
            uint8_t const * backup = b;
            if(b < e) {
                uint8_t type8 = kDataType_Invalid;
                result = detail::consume(b, e, type8);
                int64_t count = 0;
                result = result && detail::consume(b, e, count);
                map = new mapped_value();
                for(int64_t i = 0; result && i < count; ++i) {
                    key_type k;              
                    result = result && detail::consume(b, e, k);
                    value_type v;
                    result = result && detail::consume(b, e, v);                    
                    (*map->data_)[k] = v;
                }
            }
            if(!result) {
                b = backup;
            }
            else {
                target = map;
            }
            return result;
        }

        void append(std::vector<uint8_t> & target) const {
            push<int64_t>(target, type());
            push<int64_t>(target, data_->size());            
            for(map_type::const_iterator it = data_->begin(), end = data_->end(); it != end; ++it) {
                it->first->append(target);
                it->second->append(target);
            }
        }
        bool equals(base_type const & rhs) const {            
            return rhs.type() == type()
                && data_.get() != 0 
                && static_cast<mapped_value const &>(rhs).data_.get() != 0 
                && (data_.get()) == static_cast<mapped_value const &>(rhs).data_.get();
        }
        
        size_t hash() const {
            return size_t(data_.get());
        }
    protected:
        ref_ptr<map_type> data_;
    };
 
    typedef mapped_value<ref_ptr<string_value> > object_value;
    typedef mapped_value<value_ptr> map_value;


    bool consume(uint8_t const *& b, uint8_t const * e, value_ptr & target) {
        bool result = b < e;
        if(result) {
            switch(*b) {
            case kDataType_Binary:
                {
                    ref_ptr<binary_value> starget;
                    result = binary_value::consume(b, e, starget);
                    if(result) {
                        target = starget;
                    }
                }                
                break;
            case kDataType_Byte:
                {
                    ref_ptr<byte_value> starget;
                    result = byte_value::consume(b, e, starget);
                    if(result) {
                        target = starget;
                    }
                }                
                break;
            case kDataType_DateTime:
                {
                    ref_ptr<datetime_value> starget;
                    result = datetime_value::consume(b, e, starget);
                    if(result) {
                        target = starget;
                    }
                }                
                break;
            case kDataType_Double:
                {
                    ref_ptr<double_value> starget;
                    result = double_value::consume(b, e, starget);
                    if(result) {
                        target = starget;
                    }
                }                
                break;
            case kDataType_Int32:
                {
                    ref_ptr<int32_value> starget;
                    result = int32_value::consume(b, e, starget);
                    if(result) {
                        target = starget;
                    }
                }                
                break;
            case kDataType_Int64:
                {
                    ref_ptr<int64_value> starget;
                    result = int64_value::consume(b, e, starget);
                    if(result) {
                        target = starget;
                    }
                }                
                break;
            case kDataType_List:
                {
                    ref_ptr<list_value> starget;
                    result = list_value::consume(b, e, starget);
                    if(result) {
                        target = starget;
                    }
                }                
                break;
            case kDataType_Map:
                {
                    ref_ptr<map_value> starget;
                    result = map_value::consume(b, e, starget);
                    if(result) {
                        target = starget;
                    }
                }                
                break;
            case kDataType_Null:
                {
                    ref_ptr<null_value> starget;
                    result = null_value::consume(b, e, starget);
                    if(result) {
                        target = starget;
                    }
                }                
                break;
            case kDataType_Object:
                {
                    ref_ptr<object_value> starget;
                    result = object_value::consume(b, e, starget);
                    if(result) {
                        target = starget;
                    }
                }                
                break;
            case kDataType_String:
                {
                    ref_ptr<string_value> starget;
                    result = string_value::consume(b, e, starget);
                    if(result) {
                        target = starget;
                    }
                }                
                break;
            }
        }
        return false;
    }
}

