#pragma once
 // compile time type checkings
#include "common.hpp"
#include <memory>
namespace BinSerde
{
    template <typename T>
    class BinSer : public BinSerdeInterface<T> // Binary serialization
    {
        using Bufs = std::vector<std::unique_ptr<byte*>>;
    public:
        BinSer() 
        {
            bufs.clear();
            it = bufs.begin();
        }
        void operator&(T rhs)
        {

        }

        template <serdeable_class U>
        void operator<<(U rhs)
        {
            rhs.serde(this);
        }
        Bufs bufs;
        Bufs::iterator it;
    };

    template <typename T>
    class BinDe : public BinSerdeInterface<T> // Binary deserialization
    {
        void operator&(T &rhs)
        {
        }
    };

    namespace CustomSerde
    {
    } // reserved for non-instrusive serde template functions (TODO)

    // using universal refs / const lvalue refs to avoid trivial copy construction!
    inline namespace Serialization
    {
        // if actual=true, then actually write to the buffer;
        // otherwise, just return the size.
        // This is useful for pre-checking of the size before performing serialization.
        template <simple T> int serialize2buf(byte *buf, const T &object, bool actual = true);
        template <is_pair_like T> int serialize2buf(byte *buf, const T &object, bool actual = true);
        template <is_pointer_like T> int serialize2buf(byte *buf, const T &object, bool actual = true);
        // template <is_map_like T> int serialize2buf(byte *buf, const T &object, bool actual = true);
        template <container T> int serialize2buf(byte *buf, const T &object, bool actual = true);
        template <typename T> int serialize2buf(byte *buf, const SizedPair<T> &sp, bool actual = true);

        template <simple T>
        int serialize2buf(byte *buf, const T &object, bool actual)
        {
            std::cout << "ser simple" << std::endl;
            int size = sizeof(object);
            if (actual)
                memcpy(buf, (byte *)&object, sizeof(object));
            return size;
        }

        template <is_pair_like T>
        int serialize2buf(byte *buf, const T &object, bool actual)
        {
            std::cout << "ser pair" << std::endl;
            int size = 0;
            size += serialize2buf(buf + size, object.first, actual);
            size += serialize2buf(buf + size, object.second, actual);
            return size;
        }

        template <is_pointer_like T>
        int serialize2buf(byte *buf, const T &object, bool actual) 
        {
            std::cout << "ser ptr" << std::endl;
            return serialize2buf(buf, (*object), actual);
        }
        
        template <typename T>
        int serialize2buf(byte *buf, const SizedPair<T> &sp, bool actual)
        {
            // this serialize a sequence of items with each size=sizeof(object)
            std::cout << "ser sized pair" << std::endl;
            int size = 0;
            size += serialize2buf(buf + size, sp.size, actual);
            for (int ofs = 0; ofs < sp.size; ofs++)
                size += serialize2buf(buf + size, sp.elem + size, actual);
            return size;
        }

        template <container T>
        int serialize2buf(byte *buf, const T &object, bool actual)
        {
            std::cout << "ser container" << std::endl;
            int size = 0;
            int cnt = object.size();
            size += serialize2buf(buf + size, cnt, actual);
            for (auto item : object)
            {
                size += serialize2buf(buf + size, item, actual);
            }
            return size;
        }
    }
    inline namespace Deserialization
    {   
        template <simple T> int deserialize_from(byte *buf, T &object);
        template <is_pair_like T> int deserialize_from(byte *buf, T &object);
        template <is_pointer_like T> int deserialize_from(byte *buf, const T &object);
        template <is_map_like T> int deserialize_from(byte *buf, const T &object);
        template <is_normal_container T> int deserialize_from(byte *buf, T &object);

        template <simple T>
        int deserialize_from(byte *buf, T &object)
        {
            memcpy(&object, buf, sizeof(object));
            return sizeof(object);
        }

        template <is_pair_like T>
        int deserialize_from(byte *buf, T &object)
        {
            int cnt;
            int size = 0;
            size += deserialize_from(buf + size, object.first);
            size += deserialize_from(buf + size, object.second);
            return size;
        }

        template <is_pointer_like T>
        int serialize2buf(byte *buf, const T &object) 
        {
            return serialize2buf(buf, (*object));
        }
        
        // NOTE: We break the container concept into 2 parts: is_map_like(k-v) or is_normal_container(single value)
        // Reasons: see is_normal_container func.
        template <is_map_like T>
        int deserialize_from(byte *buf, T &object)
        {
            int cnt;
            int size = 0;
            size += deserialize_from(buf, cnt);
            for (int i=0;i<cnt;i++) {
                std::remove_const_t<typename T::key_type> key_deconst; 
                std::remove_const_t<typename T::mapped_type> value_deconst; 
                size += deserialize_from(buf + size, key_deconst);
                size += deserialize_from(buf + size, value_deconst);
                object.insert(object.end(), std::make_pair(key_deconst, value_deconst));
            }
            return size;
        }

        template <is_normal_container T>
        int deserialize_from(byte *buf, T &object)
        {
            int cnt;
            int size = 0;
            size += deserialize_from(buf, cnt);
            for (int i=0;i<cnt;i++) {
                typename T::value_type elem; 
                // NOTE: this works for most containers, but for some containers like `map`, it returns a k-v pair 
                // where the key is a const type. Then this function fails.
                size += deserialize_from(buf + size, elem);
                object.insert(object.end(), elem);
            }
            return size;
        }
    }

}