// Common header file including useful utils for ToySerde
// Concepts definitions, interface definitions
#pragma once
#include <string>
#include <cstring>
#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <type_traits>
#include <memory>

#define NVP(x) (Serde::NameValuePair(#x, &(x)));
#define NAMED_NVP(x,y) (Serde::NameValuePair(#x, &(y)));

namespace Serde {// flags
    using byte = unsigned char;
    const int SERDE_XML = 1; // use XML format(binary format if not specified) 
    const int SERDE_B64 = 2; // use base64 encoding(raw string if not specified), only applied to strings in XML 

    // Forward element by checking l/r value property of its container.
    // Usage: f(forward_item<T>(v[i]));
    struct element_be_forwarded {
        template<class T, class U>
        using type = std::conditional_t<std::is_lvalue_reference<T>::value, std::remove_reference_t<U>&, std::remove_reference_t<U>&&>;
    };

    template<class T, class U>
    element_be_forwarded::type<T, U> forward_item(U&& u) {
        return std::forward<element_be_forwarded::type<T, U>>(std::forward<U>(u));
    }


    // NOTE: we use `decay` to ignore const/volatile and refs
    template <typename T>
    concept simple = std::is_arithmetic_v<std::decay_t<T>>; // supported basic types (can recover from bytes array)

    template <typename T, typename... U>               // fold expressions
    concept is_any_of = (std::is_same_v<std::decay_t<T>, U> || ...); // check if the type is any of the

    template <typename T>
    concept is_pair_like = std::same_as<std::decay_t<T>, std::pair<typename T::first_type, typename T::second_type>> ||
        requires (T x) { x.first; x.second; };

    template <typename T>
    concept is_str_like = std::is_convertible_v<std::decay_t<T>, std::string_view>; // can convert to string view
    // ... Also a container!

    template <typename T>
    concept is_pointer_like = requires(std::decay_t<T> x) { // including support for smart pointers
        (*x);
        x.get();
    };

    template <typename T>
    concept container = requires(std::decay_t<T> x) {
        x.begin(); // iters
        x.end();
        x.cbegin(); // const iters
        x.cend();
        x.size();
        {
            x.empty()
        } -> std::same_as<bool>;
        x.insert(x.end(), *(x.begin())); // can be inserted (! not push_back; and forward_list is an exception)
    };

    template <typename T>
    concept is_map_like = container<T> && requires(std::decay_t<T> x) { // k-v containers, A.K.A. map/unordered_map in STL library
        requires std::same_as<typename T::value_type, std::pair<typename std::add_const_t<typename T::key_type>, typename T::mapped_type> >;
    };

    template <typename T>
    concept is_normal_container = container<T> && !is_map_like<T>;

    template <typename T>
    class SerdeInterface; // Base class for serialization/deserializzation interfaces



    // helper template to figure out the size of a fixed array
    // usage e.g.: array_size<&a>::value
    template <typename T, std::size_t N>
    constexpr std::size_t array_size(const T(&)[N]) { return N; }

    template <auto A>
    struct ArraySize
    {
        enum { value = array_size(*A) };
    };
    template <typename T>
    class SerdeInterface
    {
    public:
        SerdeInterface() = default;
        void operator&(T a) { static_assert("Not implemented"); };
        void operator<<(T a) { static_assert("Not implemented"); };
        void operator>>(T a) { static_assert("Not implemented"); };
    };

    template <typename T>
    concept serdeable_class = requires(T x) {
        {
            x.serde(std::declval<SerdeInterface<T>&>())
        } -> std::same_as<void>;
    }; // class that implements the interface

    template <typename T>
    concept supported_type = serdeable_class<T> || simple<T> || is_any_of<T> || is_pair_like<T> ||
        is_pointer_like<T> || container<T>;

    // Helper utils, reserve space for a container if possible.
    template <typename T>
    concept can_reserve = requires (T x) {
        x.reserve(0);
    };

    template <typename T> // can reserve: reserve and return true
        requires (can_reserve<T>)
    bool try2reserve(T&& container, std::size_t n) { container.reserve(n); return true; };

    template <typename T> // otherwise do nothing but return false
        requires (!can_reserve<T>)
    bool try2reserve([[maybe_unused]] T&& container, [[maybe_unused]] std::size_t n) { return false; };

    // Helper classes
    template <typename T>
    class NameValuePair { // helper class to get the name of member variables
    public:
        NameValuePair(std::string name, T& value) : name(name), value(&value) {}
        NameValuePair(std::string name, T* value) : name(name), value(value) {}
        std::string name;
        T* value;
    };

    // we don't use the std::pair to avoid confusing multiple matches in template substitution
    template <typename T>
    class SizedPair { // !not type-safe, take it carefully...
    public:
        SizedPair(T* elem, std::size_t size) : elem(elem), size(size) {}
        SizedPair(T& elem, std::size_t size) : elem(&elem), size(size) {}
        T* elem{ nullptr };
        std::size_t size{ 0 }; // elements count
    };



    // utils for base64 encoding/decoding
    // Author: Jouni Malinen
    // Website: http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
    // It's ok to use a fast decoder/encoder for my assignment, isn't it?
    static const unsigned char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string b64encode(const unsigned char* src, size_t len)
    {
        unsigned char* out, * pos;
        const unsigned char* end, * in;

        size_t olen;

        olen = 4 * ((len + 2) / 3); /* 3-byte blocks to 4-byte */

        if (olen < len)
            return std::string(); /* integer overflow */

        std::string outStr;
        outStr.resize(olen);
        out = (unsigned char*)&outStr[0];

        end = src + len;
        in = src;
        pos = out;
        while (end - in >= 3) {
            *pos++ = base64_table[in[0] >> 2];
            *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
            *pos++ = base64_table[in[2] & 0x3f];
            in += 3;
        }

        if (end - in) {
            *pos++ = base64_table[in[0] >> 2];
            if (end - in == 1) {
                *pos++ = base64_table[(in[0] & 0x03) << 4];
                *pos++ = '=';
            }
            else {
                *pos++ = base64_table[((in[0] & 0x03) << 4) |
                    (in[1] >> 4)];
                *pos++ = base64_table[(in[1] & 0x0f) << 2];
            }
            *pos++ = '=';
        }

        return outStr;
    }

    static const int B64index[256] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
    7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,
    0,  0,  0, 63,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };

    std::string b64decode(const void* data, const size_t len)
    {
        unsigned char* p = (unsigned char*)data;
        int pad = len > 0 && (len % 4 || p[len - 1] == '=');
        const size_t L = ((len + 3) / 4 - pad) * 4;
        std::string str(L / 4 * 3 + pad, '\0');

        for (size_t i = 0, j = 0; i < L; i += 4)
        {
            int n = B64index[p[i]] << 18 | B64index[p[i + 1]] << 12 | B64index[p[i + 2]] << 6 | B64index[p[i + 3]];
            str[j++] = n >> 16;
            str[j++] = n >> 8 & 0xFF;
            str[j++] = n & 0xFF;
        }
        if (pad)
        {
            int n = B64index[p[L]] << 18 | B64index[p[L + 1]] << 12;
            str[str.size() - 1] = n >> 16;

            if (len > L + 2 && p[L + 2] != '=')
            {
                n |= B64index[p[L + 2]] << 6;
                str.push_back(n >> 8 & 0xFF);
            }
        }
        return str;
    }

}