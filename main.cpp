#include "toyserde.hpp"
#include <map>
#include <unordered_map>
#include <stack>
#include <vector>
#include <cassert>
using namespace std;
int main()
{
    char buf[16384];

    // ------------------------------basic types and nested containers, raw interfaces
    string sa = "112233";
    string sb;
    BinSerde::serialize2buf((BinSerde::byte *)buf, sa);
    BinSerde::deserialize_from((BinSerde::byte *)buf, sb);
    assert(sa==sb);
    cout << sb << endl;

    auto ia = 1;
    int ib;
    BinSerde::serialize2buf((BinSerde::byte *)buf, ia);
    BinSerde::deserialize_from((BinSerde::byte *)buf, ib);
    assert(ia==ib);
    cout << ib << endl;

    pair<string, int> pa = make_pair("114", 514);
    pair<string, int> pb;
    BinSerde::serialize2buf((BinSerde::byte *)buf, pa);
    BinSerde::deserialize_from((BinSerde::byte *)buf, pb);
    assert(pa==pb);
    cout << "(" << pb.first << "," << pb.second << ")" << endl;

    vector<vector<string>> vva{vector<string>{"Girimi", "Mahiru", "Nana7mi", "Azusa"},
                               vector<string>{"21", "22", "23", "24"}};
    vector<vector<string>> vvb;
    BinSerde::serialize2buf((BinSerde::byte *)buf, vva);
    BinSerde::deserialize_from((BinSerde::byte *)buf, vvb);
    assert(vva==vvb);
    cout << vvb[0][2] << endl;

    set<string> seta{"this", "is", "a", "container", "in", "a", "set"};
    set<string> setb;
    BinSerde::serialize2buf((BinSerde::byte *)buf, seta);
    BinSerde::deserialize_from((BinSerde::byte *)buf, setb);
    assert(seta==setb);
    cout << setb.count("set") << endl;
    // -------------------------------------------------------------------------------

    // ----------------------------------------custom classes, encapsulated interfaces
    
    // -------------------------------------------------------------------------------
    // ------------------------------------+smart pointers, Binary or XML(with Base64)
    return 0;
}