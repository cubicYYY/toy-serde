#pragma once
#include "binserde.hpp"
#include "xmlserde.hpp"
#include <iostream>
#include <fstream>

#define MAX_SERDE_SIZE 16777216
#define SERDE_HEADER_SIZE 16
#define SERDE_MAGIC_NUM 0x21452505
#define SERDE_VERSION 1

namespace Serde::BinSerde {
    void fill_header(void* buf, int flags = 0, int size = 0)
    {
        int* header = reinterpret_cast<int*>(buf);
        header[0] = SERDE_MAGIC_NUM;
        header[1] = SERDE_VERSION;
        header[2] = flags;
        header[3] = size;
    }
    template <typename T>
    bool serialize(T&& object, std::string filename, int flags = 0)
    {

        // 1. Write in the header field (for metadata)
        // 2. Recursively serialize
        std::ofstream fout;
        if (flags & Serde::SERDE_B64) {
            fout.open(filename, std::ios::out | std::ios::trunc);
        }
        else fout.open(filename, std::ios::binary | std::ios::out | std::ios::trunc);

        if (!fout) throw std::logic_error("Cannot open \"" + filename + "\". Is it being occupied?");
        // header
        int size = Serde::BinSerde::serialize2buf(nullptr, std::forward<T>(object), false); // get size
        if (size <= 0 || size >= MAX_SERDE_SIZE) throw std::logic_error("Invalid serialization size.");
        std::unique_ptr<Serde::byte> buf(new Serde::byte[size + 1]);
        Serde::BinSerde::serialize2buf(buf.get(), std::forward<T>(object));
        buf.get()[size] = '\0';
        if (flags & Serde::SERDE_B64) {
            auto base64ed = Serde::b64encode((unsigned char*)(buf.get()), size);
            fout.write(base64ed.c_str(), base64ed.size());
        }
        else {
            Serde::byte header[SERDE_HEADER_SIZE];
            fill_header(&header, flags, size);
            fout.write((char*)header, SERDE_HEADER_SIZE);
            fout.write((char*)(buf.get()), size);
        }
        fout.close();
        return true;
    }

    template <typename T>
    bool deserialize(T&& object, std::string filename, int flags = 0)
    {
        // 1. Decode the header field (for metadata)
        // 2. Recursively deserialize
        std::ifstream fin;
        if (flags & Serde::SERDE_B64) fin.open(filename, std::ios::in);
        else fin.open(filename, std::ios::binary | std::ios::in);
        if (!fin) throw std::logic_error("Cannot open \"" + filename + "\". Is it being occupied?");

        if (flags & Serde::SERDE_B64) {
            fin.seekg(0, std::ios::end);
            std::size_t data_size = fin.tellg();
            fin.seekg(0, std::ios::beg);

            auto buf = new Serde::byte[data_size + 1];
            fin.read((char*)buf, data_size);
            std::string recovered = Serde::b64decode((unsigned char*)buf, data_size);
            Serde::BinSerde::deserialize_from((Serde::byte*)recovered.c_str(), std::forward<T>(object));
            fin.close();
            return true;
        }

        Serde::byte header_buf[SERDE_HEADER_SIZE];
        fin.read((char*)header_buf, SERDE_HEADER_SIZE);
        int* header = reinterpret_cast<int*>(header_buf);
        int magic = header[0];
        if (magic != SERDE_MAGIC_NUM) {
            std::cout << "Invalid file format." << std::endl;
            return false;
        }
        int version = header[1];
        if (version != SERDE_VERSION) {
            std::cout << "Invalid version: " << version << "." << std::endl;
            return false;
        }
        int flags_read = header[2];
        if (flags != flags_read) {
            std::cout << "Flags mismatched. " << std::endl;
            fin.close();
            return false;
        }
        int data_size = header[3];
        auto buf = new Serde::byte[data_size + 1];
        fin.read((char*)buf, data_size);
        Serde::BinSerde::deserialize_from(buf, std::forward<T>(object));
        fin.close();
        return true;
    }
}

namespace Serde::XmlSerde {
    // TODO: appending mode to save multiple data in one file :)
    template <typename T>
    bool serialize(T&& object, const char* obj_name, const char* filename, int flags = 0)
    {
        tinyxml2::XMLDocument doc;
        doc.InsertEndChild(doc.NewElement("serialization"));
        serialize2xml(doc, doc.RootElement(), std::forward<T>(object), obj_name, flags);
        // tinyxml2::XMLPrinter printer;
        // doc.Accept( &printer );
        // std::string stringBuffer = printer.CStr();
        // std::cout<<stringBuffer.c_str()<<std::endl;
        return tinyxml2::XML_SUCCESS == doc.SaveFile(filename);
    }

    template <typename T>
    bool deserialize(T&& object, [[maybe_unused]] const char* obj_name, const char* filename, int flags = 0)
    {
        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(filename) != tinyxml2::XML_SUCCESS) return false;
        deserialize_from(doc.RootElement()->FirstChildElement(), std::forward<T>(object), flags);
        // tinyxml2::XMLPrinter printer;
        // doc.Accept( &printer );
        // std::string stringBuffer = printer.CStr();
        // std::cout<<stringBuffer.c_str()<<std::endl;
        return true;
    }
}