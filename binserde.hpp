#pragma once
// compile time type checkings
#include "common.hpp"
#include <memory>
namespace BinSerde
{

    namespace CustomSerde
    {
    } // reserved for non-instrusive serde template functions (TODO)

    // using universal refs / const lvalue refs to avoid trivial copy construction!
    inline namespace Serialization
    {
        // if actual=true, then actually write to the buffer;
        // otherwise, just return the size.
        // This is useful for pre-checking of the size before performing serialization.
        template <typename T>
        class BinSer; // class for
        template <simple T>
        int serialize2buf(byte *buf, const T &object, bool actual = true);
        template <is_pair_like T>
        int serialize2buf(byte *buf, const T &object, bool actual = true);
        template <is_pointer_like T>
        int serialize2buf(byte *buf, const T &object, bool actual = true);
        // template <is_map_like T> int serialize2buf(byte *buf, const T &object, bool actual = true);
        template <container T>
        int serialize2buf(byte *buf, const T &object, bool actual = true);
        template <typename T>
        int serialize2buf(byte *buf, const SizedPair<T> &sp, bool actual = true);

        template <serdeable_class T>
        int serialize2buf(byte *buf, T &&object, bool actual = true); // !Not const for this

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

        template <serdeable_class T>
        int serialize2buf(byte *buf, T &&object, bool actual)
        {
            std::cout << "ser OBJ" << std::endl;
            auto archive = BinSerde::BinSer<T>(buf, actual, std::size_t(0));
            object.serde(archive);
            return archive.cursor;
        }

        template <typename T>
        class BinSer : public BinSerdeInterface<T> // Binary serialization
        {
        public:
            BinSer(byte *buf = nullptr, bool ready = false, std::size_t cursor = 0) : buf(buf), ready(ready), cursor(cursor) {}
            ~BinSer() = default;

            template <typename U>
            BinSer<T> operator&(const U &rhs)
            {
                BinSer<T> tmp(buf, ready, cursor);
                cursor += serialize2buf(buf + cursor, rhs, ready);
                tmp.cursor = cursor;
                return tmp;
            }

        public:
            byte *buf{nullptr}; // expose this buf to write back
            bool ready{false};
            std::size_t cursor{0};
        };
    }
    inline namespace Deserialization
    {
        template <typename T>
        class BinDe;
        template <simple T>
        int deserialize_from(byte *buf, T &object);
        template <is_pair_like T>
        int deserialize_from(byte *buf, T &object);
        template <is_pointer_like T>
        int deserialize_from(byte *buf, const T &object);
        template <is_map_like T>
        int deserialize_from(byte *buf, const T &object);
        template <is_normal_container T>
        int deserialize_from(byte *buf, T &object);
        template <serdeable_class T>
        int deserialize_from(byte *buf, T &&object);

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

        // NOTE: We break the container concept into 2 parts: is_map_like(k-v) or is_normal_container(single value)
        // Reasons: see is_normal_container func.
        template <is_map_like T>
        int deserialize_from(byte *buf, T &object)
        {
            int cnt;
            int size = 0;
            size += deserialize_from(buf, cnt);
            for (int i = 0; i < cnt; i++)
            {
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
            for (int i = 0; i < cnt; i++)
            {
                typename T::value_type elem;
                // NOTE: this works for most containers, but for some containers like `map`, it returns a k-v pair
                // where the key is a const type. Then this function fails.
                size += deserialize_from(buf + size, elem);
                object.insert(object.end(), elem);
            }
            return size;
        }

        template <serdeable_class T>
        int deserialize_from(byte *buf, T &&object)
        {
            std::cout << "de OBJ" << std::endl;
            auto archive = BinSerde::BinDe<T>(buf, 0);
            object.serde(archive);
            return archive.cursor;
        }

        template <is_pointer_like T> // smart pointers
        int deserialize_from(byte *buf, T &&object)
        {
            int size = 0;
            typename std::remove_reference_t<T>::element_type item; // get the inside element
            size += deserialize_from(buf, item);
            // re-allocate and initialize
            object.reset(new typename std::remove_reference_t<T>::element_type(std::move(item))); 
            return size;
        }


        template <typename T>
        class BinDe : public BinSerdeInterface<T> // Binary deserialization
        {
        public:
            BinDe(byte *buf = nullptr, std::size_t cursor = 0) : buf(buf), cursor(cursor) {}

            template <typename U>
            BinDe<T> operator&(U &rhs)
            {
                BinDe<T> tmp(buf, cursor);
                cursor += deserialize_from(buf + cursor, rhs);
                tmp.cursor = cursor;
                return tmp;
            }

        public:
            byte *buf{nullptr};
            std::size_t cursor{0};
        };
    }

}