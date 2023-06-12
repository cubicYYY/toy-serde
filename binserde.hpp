#pragma once
#include <string>
#include <cstring>
#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <type_traits> // compile time type checkings

namespace BinSerde
{
    using byte = char;
    template <typename T>
    concept simple = std::is_arithmetic_v<T> || std::is_trivially_copyable_v<T>;
    // supported basic types, which is trivially copyable (can recover from bytes array)

    template <typename T, typename... U>               // fold expressions
    concept is_any_of = (std::is_same_v<T, U> || ...); // check if the type is any of the

    template <typename T>
    concept is_pair_like = std::same_as<T, std::pair<typename T::first_type, typename T::second_type>>;

    template <typename T>
    concept is_str_like = std::is_convertible_v<T, std::string_view>; // can convert to string view

    template <typename T>
    concept is_pointer_like = requires(T x) { // including support for smart pointers
        (*x);
        x.get();
    };

    template <typename T>
    concept container = requires(T x) {
        x.begin(); // iters
        x.end();
        x.cbegin(); // const iters
        x.cend();
        x.size();
        {
            x.empty()
        } -> std::same_as<bool>;
        x.insert(x.end(), *(x.begin())); // can be inserted (! not push_back, forward_list is an exception)
    };

    template <typename T>
    class BinSerdeInterface // Base class for serialization/deserializzation interfaces
    {
        BinSerdeInterface() = default;
        void operator&(T a) { static_assert("Not implemented"); };
        void operator<<(T a) { static_assert("Not implemented"); };
        void operator>>(T a) { static_assert("Not implemented"); };
    };

    template <typename T>
    concept serdeable = requires(T x) {
        {
            x.serde(declval(std::forward<BinSerdeInterface<T>>))
        } -> std::same_as<void>;
    }; // class that implements the interface

    template <typename T>
    class BinSer : public BinSerdeInterface<T>
    {
    public:
        void operator&(T rhs)
        {
        }

        void operator<<(T rhs)
        {
            rhs.serde(this);
        }
        byte *buf{nullptr};
    };

    template <typename T>
    class BinDe : public BinSerdeInterface<T>
    {
        void operator&(T &rhs)
        {
        }
    };

    namespace CustomSerde
    {
    } // reserved for non-instrusive serde template functions

    // using universal references to avoid trivial copy construction!
    inline namespace Serialization
    {
        // if actual=true, then actually write to the buffer;
        // otherwise, just return the size.
        // This is useful for pre-checking of the size before performing serialization.
        template <simple T>
        int serialize2buf(void *buf, T &object, bool actual = true)
        {
            std::cout << "sint";
            int size = sizeof(T);
            if (actual)
                memcpy(buf, (void *)&object, sizeof(T));
            std::cout << size << std::endl;
            return size;
        }

        template <container T>
        int serialize2buf(void *buf, T &object, bool actual = true)
        {
            std::cout << "scont";
            int size = 0;
            int cnt = object.size();
            size += serialize2buf(buf + size, cnt, actual);
            for (auto item : object)
            {
                size += serialize2buf(buf + size, item, actual);
            }
            return size;
        }

        template <is_pair_like T>
        int serialize2buf(void *buf, T &object, bool actual = true)
        {
            std::cout << "scont";
            int size = 0;
            size += serialize2buf(buf + size, object.first, actual);
            size += serialize2buf(buf + size, object.second, actual);
            return size;
        }

        template <is_pointer_like T>
        int serialize2buf(void *buf, T &object, bool actual = true)
        {
            return serialize2buf(buf, (*object), actual);
        }

        std::string b64_encode(byte *raw, int len)
        {
            return "";
        }
    }
    inline namespace Deserialization
    {
        template <simple T>
        void deserialize_from(void *buf, T& object)
        {
            memcpy(&object, buf, sizeof(T));
        }

        std::string b64_decode(std::string encoded)
        {
        }
    }

}