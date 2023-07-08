#pragma once
// TODO: using static_assert to generate more readable compiling errors
// TODO: forward items of a object to avoid redundant copy/moves
// TODO: Wrap the buf into a class
// compile time type checkings
#include "common.hpp"
namespace Serde::BinSerde {

    namespace CustomSerde // TODO
    {
    } // reserved for non-instrusive serde template functions (TODO)

    // using universal refs / const lvalue refs to avoid trivial copy construction!
    inline namespace Serialization
    {
        // if actual=true, then actually write to the buffer;
        // otherwise, just return the size.
        // This is useful for pre-checking of the size before performing serialization.
        // Use universal references for objects, but pass-by-value for wrapper pairs

        // declarations
        template <typename T>
        class BinSer; // class for binary serde
        template <simple T>
        int serialize2buf(Serde::byte* buf, T&& object, bool actual = true);
        template <is_pair_like T>
        int serialize2buf(Serde::byte* buf, T&& object, bool actual = true);
        template <is_pointer_like T>
        int serialize2buf(Serde::byte* buf, T&& object, bool actual = true);
        template <container T> // !including std::string type
        int serialize2buf(Serde::byte* buf, T&& object, bool actual = true);
        template <typename T>
        int serialize2buf(Serde::byte* buf, SizedPair<T> sp, bool actual = true);
        template <typename T>
        int serialize2buf(Serde::byte* buf, NameValuePair<T> nvp, bool actual = true);

        template <serdeable_class T>
        int serialize2buf(Serde::byte* buf, T&& object, bool actual = true);

        // implementations
        template <simple T>
        int serialize2buf(Serde::byte* buf, T&& object, bool actual)
        {
            int size = sizeof(object);
            if (actual)
                memcpy(buf, static_cast<const void*>(&object), sizeof(object));
            return size;
        }

        template <is_pair_like T>
        int serialize2buf(Serde::byte* buf, T&& object, bool actual)
        {
            int size = 0;
            size += serialize2buf(buf + size, Serde::forward_item<T>(object.first), actual);
            size += serialize2buf(buf + size, Serde::forward_item<T>(object.second), actual);
            return size;
        }

        template <is_pointer_like T>
        int serialize2buf(Serde::byte* buf, T&& object, bool actual)
        {
            return serialize2buf(buf, (*std::forward<T>(object)), actual);
        }


        template <container T>
        int serialize2buf(Serde::byte* buf, T&& object, bool actual)
        {
            int size = 0;
            int cnt = object.size();
            size += serialize2buf(buf + size, cnt, actual);
            for (auto&& item : object)
            {
                size += serialize2buf(buf + size, item, actual);
            }
            return size;
        }

        template <typename T>
        int serialize2buf(Serde::byte* buf, const SizedPair<T> sp, bool actual)
        {
            // this serialize a sequence of items with each size=sizeof(object)
            int size = 0;
            size += serialize2buf(buf + size, sp.size, actual);
            for (auto ofs = 0u; ofs < sp.size; ofs++)
                size += serialize2buf(buf + size, *(sp.elem + ofs), actual);
            return size;
        }

        template <serdeable_class T>
        int serialize2buf(Serde::byte* buf, T&& object, bool actual)
        {
            auto archive = BinSerde::BinSer<T>(buf, actual, std::size_t(0));
            object.serde(archive);
            return archive.cursor;
        }

        template <typename T>
        int serialize2buf(Serde::byte* buf, NameValuePair<T> object, bool actual)
        {
            return serialize2buf(buf, *(object.value), actual); // we just drop the name in binary mode.
        }



        template <typename T>
        class BinSer : public SerdeInterface<T> // Binary serialization helper class for custom types
        {
        public:
            BinSer(Serde::byte* buf = nullptr, bool ready = false, std::size_t cursor = 0) : buf(buf), ready(ready), cursor(cursor) {}
            ~BinSer() = default;

            template <typename U>
            BinSer<T> operator&(U&& rhs)
            {
                BinSer<T> tmp(buf, ready, cursor);
                cursor += serialize2buf(buf + cursor, rhs, ready);
                tmp.cursor = cursor;
                return tmp;
            }

        public:
            Serde::byte* buf{ nullptr }; // expose this buf to write back
            bool ready{ false };
            std::size_t cursor{0};
        };
    }
    inline namespace Deserialization
    {
        // declarations
        template <typename T>
        class BinDe;
        template <simple T>
        int deserialize_from(Serde::byte* buf, T& object);
        template <is_pair_like T>
        int deserialize_from(Serde::byte* buf, T& object);
        template <is_pointer_like T>
        int deserialize_from(Serde::byte* buf, T& object);
        template <is_map_like T>
        int deserialize_from(Serde::byte* buf, T& object);
        template <is_normal_container T>
        int deserialize_from(Serde::byte* buf, T& object);
        template <serdeable_class T>
        int deserialize_from(Serde::byte* buf, T& object);
        template <typename T>
        int deserialize_from(Serde::byte* buf, SizedPair<T> object);
        template <typename T>
        int deserialize_from(Serde::byte* buf, NameValuePair<T> object);

        // implementations
        template <simple T>
        int deserialize_from(Serde::byte* buf, T& object)
        {
            memcpy(&object, buf, sizeof(object));
            return sizeof(object);
        }

        template <is_pair_like T>
        int deserialize_from(Serde::byte* buf, T& object)
        {
            int size = 0;
            size += deserialize_from(buf + size, object.first);
            size += deserialize_from(buf + size, object.second);
            return size;
        }

        // NOTE: We break the container concept into 2 parts: is_map_like(k-v) or is_normal_container(single value)
        // Reasons: see is_normal_container func.
        template <is_map_like T>
        int deserialize_from(Serde::byte* buf, T& object)
        {
            int cnt = 0;
            int size = 0;
            size += deserialize_from(buf, cnt);
            try2reserve(object, cnt); // !optimized :)
            for (int i = 0; i < cnt; i++)
            {
                typename std::remove_const_t<typename T::key_type> key_deconst;
                typename T::mapped_type value_deconst;
                size += deserialize_from(buf + size, key_deconst);
                size += deserialize_from(buf + size, value_deconst);
                object.insert(object.end(), std::make_pair(key_deconst, value_deconst));
            }
            return size;
        }

        template <is_normal_container T>
        int deserialize_from(Serde::byte* buf, T& object)
        {
            int cnt = 0;
            int size = 0;
            size += deserialize_from(buf, cnt);
            try2reserve(object, cnt); // !optimized :)
            for (int i = 0; i < cnt; i++)
            {
                typename std::remove_const_t<typename T::value_type> elem;
                // NOTE: this works for most containers, but for some containers like `map`, it returns a k-v pair
                // where the key is a const type. Then this function fails.
                size += deserialize_from(buf + size, elem);
                object.insert(object.end(), elem);
            }
            return size;
        }

        template <serdeable_class T>
        int deserialize_from(Serde::byte* buf, T& object)
        {
            auto archive = BinSerde::BinDe<T>(buf, 0);
            object.serde(archive);
            return archive.cursor;
        }

        template <is_pointer_like T> // smart pointers
        int deserialize_from(Serde::byte* buf, T& object)
        {
            int size = 0;
            typename std::remove_reference_t<T>::element_type item; // get the inside element
            size += deserialize_from(buf, item);
            // re-allocate and initialize
            object.reset(new typename std::remove_reference_t<T>::element_type(std::move(item)));
            return size;
        }

        template <typename T>
        int deserialize_from(Serde::byte* buf, NameValuePair<T> object)
        {
            return deserialize_from(buf, *(object.value));
        }

        template <typename T>
        int deserialize_from(Serde::byte* buf, SizedPair<T> object)
        {
            int size = 0;
            std::size_t cnt;
            size += deserialize_from(buf + size, cnt);
            for (auto i = 0u;i < cnt;i++)
                size += deserialize_from(buf + size, *(object.elem + i));
            return size;
        }

        template <typename T>
        class BinDe : public SerdeInterface<T> // Binary deserialization
        {
        public:
            BinDe(Serde::byte* buf = nullptr, std::size_t cursor = 0) : buf(buf), cursor(cursor) {}

            template <typename U>
            BinDe<T> operator&(U&& rhs)
            {
                BinDe<T> tmp(buf, cursor);
                cursor += deserialize_from(buf + cursor, rhs);
                tmp.cursor = cursor;
                return tmp;
            }

        public:
            Serde::byte* buf{ nullptr };
            std::size_t cursor{0};
        };
    }


}