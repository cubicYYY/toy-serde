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

namespace Serde{// flags
    using byte = char;
const int SERDE_XML = 1; // use XML format(binary format if not specified) 
const int SERDE_B64 = 2; // use base64 encoding(raw string if not specified), only applied to strings in XML 

#define NVP(x) (Serde::NameValuePair(#x, &(x)));
#define NAMED_NVP(x,y) (Serde::NameValuePair(#x, &(y)));

template <typename T>
concept simple = std::is_arithmetic_v<T>; // supported basic types (can recover from bytes array)

template <typename T, typename... U>               // fold expressions
concept is_any_of = (std::is_same_v<T, U> || ...); // check if the type is any of the

template <typename T>
concept is_pair_like = std::same_as<T, std::pair<typename T::first_type, typename T::second_type>> ||
    requires (T x) { x.first; x.second;};

// template <typename T>
// concept is_str_like = std::is_convertible_v<T, std::string_view>; // can convert to string view
// ... Also a container!

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
concept is_map_like = container<T> && requires(T x) { // k-v containers, A.K.A. map/unordered_map in STL library
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

template <typename T>
class NameValuePair { // helper class to get the name of member variables
public:
    NameValuePair(std::string name, T &value) : name(name), value(&value) {}
    NameValuePair(std::string name, T* value) : name(name), value(value) {}
    std::string name;
    T* value;
};

namespace BinSerde { // classes

    // we don't use the std::pair to avoid confusing multiple matches in template substitution
    template <typename T>
    class SizedPair { // !not type safe, take it carefully...
    public:
        SizedPair(auto elem, auto size) : elem(elem), size(size) {}
        T *&elem{nullptr};
        std::size_t size{0}; // elements count
    };
    
}

// utils for base64 encoding/decoding
class B64Cache {
public:
    B64Cache() = default;
    B64Cache(Serde::byte *buf) {

    }
    std::vector<void *> addr;
};
std::string b64_encode(Serde::byte *buf, int size)
{
}

void b64_decode(Serde::byte *buf, std::string text)
{

}

}