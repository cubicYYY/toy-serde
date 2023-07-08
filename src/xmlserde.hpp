#pragma once
#include "common.hpp"
#include "tinyxml2.h"
#include <sstream>
namespace Serde::XmlSerde {
    using XMLElement = tinyxml2::XMLElement;
    using XMLDocument = tinyxml2::XMLDocument;
    namespace CustomSerde
    {
    } // reserved for non-instrusive serde template functions (TODO)

    // using universal refs / const lvalue refs to avoid trivial copy construction!
    inline namespace Serialization
    {
        // if actual=true, then actually write to the buffer;
        // otherwise, just return the size.
        // This is useful for pre-checking of the size before performing serialization.

        // declarations
        // ! use const char* for the tag name
        // what an awful API...
        template <typename T>
        class XmlSer; // class for
        template <simple T>
        void serialize2xml(XMLDocument& doc, XMLElement* parent, T&& object, const char* key_name = nullptr, int flags = 0);
        template <is_pair_like T>
        void serialize2xml(XMLDocument& doc, XMLElement* parent, T&& object, const char* key_name = nullptr, int flags = 0);
        template <is_pointer_like T>
        void serialize2xml(XMLDocument& doc, XMLElement* parent, T&& object, const char* key_name = nullptr, int flags = 0);
        template <container T> // we need to handle string separately
            requires  (!is_str_like<T>) // exclude string type
        void serialize2xml(XMLDocument& doc, XMLElement* parent, T&& object, const char* key_name = nullptr, int flags = 0);

        template <is_str_like T> // special case of a container
        void serialize2xml(XMLDocument& doc, XMLElement* parent, T&& object, const char* key_name = nullptr, int flags = 0);

        template <typename T>
        void serialize2xml(XMLDocument& doc, XMLElement* parent, SizedPair<T> sp, const char* key_name = nullptr, int flags = 0);
        template <typename T>
        void serialize2xml(XMLDocument& doc, XMLElement* parent, NameValuePair<T> nvp, const char* key_name = nullptr, int flags = 0);
        template <typename T>
        void serialize2xml(XMLDocument& doc, XMLElement* parent, std::string& str, const char* key_name = nullptr, int flags = 0);

        template <serdeable_class T>
        void serialize2xml(XMLDocument& doc, XMLElement* parent, T&& object, const char* key_name = nullptr, int flags = 0); // !Not const for this

        // implementations
        template <simple T>
        void serialize2xml(XMLDocument& doc, XMLElement* parent, T&& object, const char* key_name, [[maybe_unused]] int flags)
        {
            XMLElement* element = doc.NewElement(key_name ? key_name : "basic");
            element->SetAttribute("val", object);
            parent->InsertEndChild(element);
        }

        template <is_pair_like T> // FIXME: should we somehow forward it here?
        void serialize2xml(XMLDocument& doc, XMLElement* parent, T&& object, const char* key_name, int flags)
        {
            XMLElement* element = doc.NewElement(key_name ? key_name : "pair");
            serialize2xml(doc, element, Serde::forward_item<T>(object.first), "first", flags);
            serialize2xml(doc, element, Serde::forward_item<T>(object.second), "second", flags);
            parent->InsertEndChild(element);
        }

        template <is_pointer_like T>
        void serialize2xml(XMLDocument& doc, XMLElement* parent, T&& object, [[maybe_unused]] const char* key_name, int flags)
        {
            serialize2xml(doc, parent, (*std::forward<T>(object)), key_name, flags); // just forward
        }

        template <container T>
            requires  (!is_str_like<T>)
        void serialize2xml(XMLDocument& doc, XMLElement* parent, T&& object, const char* key_name, int flags)
        {
            XMLElement* element = doc.NewElement(key_name ? key_name : "container");
            for (auto&& item : object)
            {
                serialize2xml(doc, element, item, "item", flags);
            }
            parent->InsertEndChild(element);
        }

        template <is_str_like T>
        void serialize2xml(XMLDocument& doc, XMLElement* parent, T&& object, const char* key_name, int flags)
        {
            Serde::byte* buf = nullptr;
            XMLElement* element = doc.NewElement(key_name ? key_name : "str");
            if (flags & SERDE_B64) {
                auto base64ed = Serde::b64encode((unsigned char*)object.c_str(), object.size());
                buf = new Serde::byte[base64ed.size() + 1];
                memcpy((char*)buf, base64ed.c_str(), base64ed.size());
                buf[base64ed.size()] = '\0'; // !don't forget the trailing 0
            }
            else {
                buf = new Serde::byte[object.size() + 1];
                memcpy((char*)buf, object.c_str(), object.size());
                buf[object.size()] = '\0';
            }

            element->InsertEndChild(doc.NewText((char*)buf)); // ptr hold & managed by tinyxml
            parent->InsertEndChild(element);
        }

        template <typename T>
        void serialize2xml(XMLDocument& doc, XMLElement* parent, const SizedPair<T> sp, const char* key_name, int flags)
        {
            XMLElement* element = doc.NewElement(key_name ? key_name : "sized_seg");
            for (auto ofs = 0u; ofs < sp.size; ofs++)
                serialize2xml(doc, element, *(sp.elem + ofs), "raw", flags);
            parent->InsertEndChild(element);
        }

        template <serdeable_class T>
        void serialize2xml(XMLDocument& doc, XMLElement* parent, T&& object, const char* key_name, int flags)
        {
            XMLElement* element = doc.NewElement(key_name ? key_name : "complex_class");
            auto archive = XmlSerde::XmlSer<T>(doc, element, flags);
            object.serde(archive);
            parent->InsertEndChild(element);
        }

        template <typename T>
        void serialize2xml(XMLDocument& doc, XMLElement* parent, NameValuePair<T> object, [[maybe_unused]] const char* key_name, int flags)
        {
            char* tmp = new char[object.name.size() + 1];
            memcpy(tmp, object.name.c_str(), object.name.size() + 1);
            serialize2xml(doc, parent, *(object.value), tmp, flags);
            // delete[] tmp;
            // !managed by tinyxml, no deletion is needed
        }



        template <typename T>
        class XmlSer : public SerdeInterface<T>
        {
        public:
            XmlSer(XMLDocument& doc, XMLElement* parent = nullptr, int flags = 0) : doc(doc), parent(parent), flags(flags) {}
            ~XmlSer() = default;

            template <typename U>
            XmlSer<T> operator&(U&& rhs)
            {
                XmlSer<T> tmp(doc, parent, flags);
                serialize2xml(doc, parent, rhs, "item", flags);
                return tmp;
            }

        public:
            XMLDocument& doc;
            XMLElement* parent{ nullptr }; // expose this buf to write back
            int flags{ 0 };
        };
    }
    inline namespace Deserialization
    {
        // declarations
        template <typename T>
        class XmlDe;
        template <simple T>
            requires (!is_any_of<typename std::remove_cv_t<typename std::remove_reference_t<T>>, char, unsigned char, bool>)
        void deserialize_from(XMLElement* cur_element, T& object, int flags = 0);
        template <typename T> // special case: char
            requires (is_any_of<typename std::remove_cv_t<typename std::remove_reference_t<T>>, char, unsigned char>)
        void deserialize_from(XMLElement* cur_element, T& object, int flags = 0);
        template <typename T> // special case: bool
            requires (is_any_of<typename std::remove_cv_t<typename std::remove_reference_t<T>>, bool>)
        void deserialize_from(XMLElement* cur_element, T& object, int flags = 0);
        template <is_pair_like T>
        void deserialize_from(XMLElement* cur_element, T& object, int flags = 0);
        template <is_pointer_like T>
        void deserialize_from(XMLElement* cur_element, T& object, int flags = 0);
        template <is_map_like T>
            requires  (!is_str_like<T>) // exclude string type
        void deserialize_from(XMLElement* cur_element, T& object, int flags = 0);
        template <is_normal_container T>
            requires  (!is_str_like<T>) // exclude string 
        void deserialize_from(XMLElement* cur_element, T& object, int flags = 0);
        template <is_str_like T> // string
        void deserialize_from(XMLElement* cur_element, T& object, int flags = 0);
        template <serdeable_class T>
        void deserialize_from(XMLElement* cur_element, T& object, int flags = 0);
        template <typename T>
        void deserialize_from(XMLElement* cur_element, SizedPair<T> object, int flags = 0);
        template <typename T>
        void deserialize_from(XMLElement* cur_element, NameValuePair<T> object, int flags = 0);

        // implementations
        template <simple T>
            requires (!is_any_of<typename std::remove_cv_t<typename std::remove_reference_t<T>>, char, unsigned char, bool>)
        void deserialize_from(XMLElement* cur_element, T& object, [[maybe_unused]] int flags)
        {
            std::stringstream(cur_element->Attribute("val")) >> object; // how elegant I am!
        }

        template <typename T>
            requires (is_any_of<typename std::remove_cv_t<typename std::remove_reference_t<T>>, char, unsigned char>)
        void deserialize_from(XMLElement* cur_element, T& object, [[maybe_unused]] int flags)
        {
            int tmp;
            std::stringstream(cur_element->Attribute("val")) >> tmp;
            object = tmp;
        }

        template <typename T>
            requires (is_any_of<typename std::remove_cv_t<typename std::remove_reference_t<T>>, bool>)
        void deserialize_from(XMLElement* cur_element, T& object, [[maybe_unused]] int flags)
        {
            std::string tmp;
            std::stringstream(cur_element->Attribute("val")) >> tmp;
            object = !(tmp == "false" || tmp == "0" || tmp == "NO");
        }

        // NOTE: We break the container concept into 2 parts: is_map_like(k-v) or is_normal_container(single value)
        // Reasons: see is_normal_container func.
        template <is_map_like T>
            requires  (!is_str_like<T>) // exclude string type
        void deserialize_from(XMLElement* cur_element, T& object, int flags)
        {
            int cnt = 0;
            for (auto elem = cur_element->FirstChildElement();elem != nullptr;elem = elem->NextSiblingElement())
                cnt++;
            try2reserve(object, cnt); // !optimized here :)
            // if the container can be reserved, then just do it for a better performance

            for (auto elem = cur_element->FirstChildElement(); elem != nullptr; elem = elem->NextSiblingElement())
            {
                typename std::remove_const_t<typename T::key_type> key_deconst;
                typename T::mapped_type value_deconst;
                deserialize_from(elem->FirstChildElement(), key_deconst, flags);
                deserialize_from(elem->LastChildElement(), value_deconst, flags);
                object.insert(object.end(), std::make_pair(key_deconst, value_deconst));
            }
        }

        template <is_normal_container T>
            requires  (!is_str_like<T>) // exclude string type
        void deserialize_from(XMLElement* cur_element, T& object, int flags)
        {
            int cnt = 0;
            for (auto elem = cur_element->FirstChildElement();elem != nullptr;elem = elem->NextSiblingElement())
                cnt++;
            try2reserve(object, cnt); // !optimized here :)
            // if the container can be reserved, then just do it for a better performance

            for (auto elem = cur_element->FirstChildElement();elem != nullptr;elem = elem->NextSiblingElement())
            {
                typename std::remove_const_t<typename T::value_type> tmp_obj;
                // NOTE: this works for most containers, but for some containers like `map`, it returns a k-v pair
                // where the key is a const type. Then this function fails.
                // So the "is_normal_container" trait is to avoid this
                deserialize_from(elem, tmp_obj, flags);
                object.insert(object.end(), tmp_obj);
            }
        }

        template <is_str_like T> // string
        void deserialize_from(XMLElement* cur_element, T& object, int flags)
        {
            if (flags & SERDE_B64) {
                auto base64ed = Serde::b64decode((unsigned char*)cur_element->GetText(), strlen(cur_element->GetText()));
                object = base64ed;
                return;
            }

            object = std::string(cur_element->GetText());

        }

        template <is_pair_like T>
        void deserialize_from(XMLElement* cur_element, T& object, int flags)
        {
            deserialize_from(cur_element->FirstChildElement(), object.first, flags);
            deserialize_from(cur_element->LastChildElement(), object.second, flags);
        }

        template <is_pointer_like T> // smart pointers
        void deserialize_from(XMLElement* cur_element, T& object, int flags)
        {
            typename std::remove_reference_t<T>::element_type item; // get the inside element
            deserialize_from(cur_element, item, flags);
            // re-allocate and initialize
            object.reset(new typename std::remove_reference_t<T>::element_type(std::move(item)));
        }

        template <typename T>
        void deserialize_from(XMLElement* cur_element, NameValuePair<T> object, int flags)
        {
            return deserialize_from(cur_element, *(object.value), flags);
        }

        template <typename T>
        void deserialize_from(XMLElement* cur_element, SizedPair<T> object, int flags)
        {
            int cnt = 0;
            for (auto cur = cur_element->FirstChildElement(); cur != nullptr; cur = cur->NextSiblingElement()) {
                deserialize_from(cur, *(object.elem + cnt), flags);
                cnt++;
            }
        }

        template <serdeable_class T>
        void deserialize_from(XMLElement* cur_element, T& object, int flags)
        {
            auto archive = XmlSerde::XmlDe<T>(cur_element, flags);
            object.serde(archive);
        }

        template <typename T>
        class XmlDe : public SerdeInterface<T>
        {
        public:
            XmlDe(XMLElement* root = nullptr, int flags = 0) : root(root), flags(flags)
            {
                child = root->FirstChildElement();
            }

            template <typename U>
            XmlDe<T> operator&(U&& rhs)
            {
                XmlDe<T> tmp(root, flags);
                if (root->NoChildren()) return tmp;
                if (child == nullptr) throw std::logic_error("Unexpected: no child left for deserialization.");
                deserialize_from(child, rhs, flags);
                child = tmp.child = child->NextSiblingElement();
                return tmp;
            }

        public:
            XMLElement* child{ nullptr };
        private:
            XMLElement* root{ nullptr };
            int flags{ 0 };
        };
    }


}