#pragma once
#include "binserde.hpp"
#include "xmlserde.hpp"
#include "common.hpp"
#include <iostream>
#include <fstream>

#define MAX_SERDE_SIZE 16777216
#define SERDE_HEADER_SIZE 16
#define SERDE_MAGIC_NUM 0x21452505
#define SERDE_VERSION 1

namespace Serde::BinSerde {
void fill_header(void *buf, int flags = 0, int size = 0)
{
    int *header = reinterpret_cast<int *>(buf);
    header[0] = SERDE_MAGIC_NUM;
    header[1] = SERDE_VERSION;
    header[2] = flags;
    header[3] = size;
}
template <typename T>
bool serialize(T &&object, std::string filename, int flags = 0)
{
    
    // 1. Write in the header field (for metadata)
    // 2. Recursively serialize
    std::ofstream fout;
    if (flags & Serde::SERDE_B64) {
        // TODO...
    } else fout.open(filename, std::ios::binary | std::ios::out);
    if (!fout) return false;
    // header
    int size = Serde::BinSerde::serialize2buf(nullptr, std::forward<T>(object), false); // get size
    if (size <= 0 || size >= MAX_SERDE_SIZE) throw std::logic_error("Invalid serialization size.");
    auto buf = new Serde::byte[size];
    Serde::BinSerde::serialize2buf(buf, std::forward<T>(object));

    Serde::byte header[SERDE_HEADER_SIZE];
    fill_header(&header, flags, size);
    if (flags & Serde::SERDE_B64) {
        // TODO...
    } else {
        fout.write((Serde::byte*)(header), SERDE_HEADER_SIZE);
        fout.write((Serde::byte*)(buf), size);
    }
    fout.close();
    delete[] buf;
    return true;
}

template <typename T>
bool deserialize(T &object, std::string filename)
{
    // 1. Decode the header field (for metadata)
    // 2. Recursively deserialize
    std::ifstream fin;
    fin.open(filename, std::ios::binary | std::ios::in);
    if (!fin) return false;
    
    Serde::byte header_buf[SERDE_HEADER_SIZE];
    fin.read(header_buf, SERDE_HEADER_SIZE);
    int *header = reinterpret_cast<int *>(header_buf);
    int magic = header[0];
    if (magic != SERDE_MAGIC_NUM) {
        std::cout << "Invalid file format." << std::endl;
        return false; 
    }
    int version = header[1];
    if (version != SERDE_VERSION) {
        std::cout << "Invalid version: " << version <<"." << std::endl;
        return false; 
    }
    [[maybe_unused]] int flags = header[2];
    int data_size = header[3];
    auto buf = new Serde::byte[data_size];
    fin.read(buf, data_size);
    Serde::BinSerde::deserialize_from(buf, object);
    fin.close();
    return true;
}
}

namespace Serde::XmlSerde {
// template <typename T>
// bool serialize(T &&object, std::string obj_name, std::string filename, int flags = 0)
// {
//     std::ofstream fout;
//     fout.open(filename, std::ios::out);
    
//     return true;
// }

// template <typename T>
// bool deserialize(T &object, std::string obj_name, std::string filename, int flags = 0)
// {
//     std::ifstream fin;
//     fin.open(filename, std::ios::in);
    
//     return true;
// }
}