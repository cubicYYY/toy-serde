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
namespace BinSerde { // concepts
        using byte = char;

    // flags
    const int SD_XML = 1; // use XML format(binary format if not specified) 
    const int SD_B64 = 2; // use base64 encoding(raw string if not specified), only applied to strings in XML 

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
        std::same_as<typename T::value_type, std::pair<typename T::key_type, typename T::mapped_type> >;
    };

    template <typename T>
    concept is_normal_container = container<T> && !is_map_like<T>;

    template <typename T>
    class BinSerdeInterface; // Base class for serialization/deserializzation interfaces
    

    template <typename T>
    concept serdeable_class = requires(T x) {
        {
            x.serde(std::declval<BinSerdeInterface<T>&>())
        } -> std::same_as<void>;
    }; // class that implements the interface

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
    concept supported_type = serdeable_class<T> || simple<T> || is_any_of<T> || is_pair_like<T> || 
                                is_pointer_like<T> || container<T>;


}
namespace BinSerde { // classes
    template <typename T>
    class BinSerdeInterface
    {
    public:
        BinSerdeInterface() = default;
        void operator&(T a) { static_assert("Not implemented"); };
        void operator<<(T a) { static_assert("Not implemented"); };
        void operator>>(T a) { static_assert("Not implemented"); };
    };

    // we don't use the std::pair to avoid confusing multiple matches in template substitution
    template <typename T>
    class SizedPair { // !not type safe, take it carefully...
    public:
        SizedPair(auto elem, auto size) : elem(elem), size(size) {}
        T *&elem{nullptr};
        std::size_t size{0}; // elements count
    };

    template <typename T>
    class NameValuePair { // helper class to get the name of member variables
    public:
        NameValuePair(auto name, auto value) : name(name), value(value) {}
        std::string name;
        T& value;
    };
    #define NVP(x) (#x, x);
}
