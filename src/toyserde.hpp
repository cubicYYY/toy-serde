#pragma once
#include "binserde.hpp"
#include "xmlserde.hpp"
#include <iostream>
#include <fstream>

#define MAX_SERDE_SIZE 16777216
template <typename T>
int serialize(T &&object, std::string filename)
{
    std::ofstream fout;
    fout.open(filename, std::ios::binary | std::ios::out);

    // header
    int size = 0;

    // TODO...

    size = Serde::BinSerde::serialize2buf(nullptr, std::forward<T>(object), false); // get size
    if (size <= 0 || size >= MAX_SERDE_SIZE) throw std::logic_error("Invalid serialization size.");
    auto buf = std::unique_ptr<char*>(new char[size]);
    Serde::BinSerde::serialize2buf(buf.get(), std::forward<T>(object));
    fout.write((char*)(buf.get()), size);
    fout.close();
}

template <typename T>
int deserialize(T &object, std::string filename)
{
    std::ifstream fin;
    fin.open(filename, std::ios::binary | std::ios::in);
    // TODO...
    fin.close();
}