#include "toyserde.hpp"
#include <map>
#include <unordered_map>
#include <stack>
#include <vector>
#include <cassert>
using namespace std;
using namespace Serde;
using namespace tinyxml2;
class Test // All-in-one test class!
{
public:
    Test(int a, int b) : a(a), b(b) { fill_test_data(); } // only construct complex containers before serialization
    Test() = default;
    template <typename A>
    void serde(A& ar)
    {
        ar& NVP(a); // ! If you don't need a XML serialization, you can ever omit the NVP()
        //Can even: ar &a &b &c &d;
        ar& NAMED_NVP(InternetOverdose, b); // custom name
        ar& NVP(complex1);
        ar& NVP(complex2);
        ar& NVP(pair_cb);
        ar& NVP(smartptr);
    }
    void fill_test_data()
    {
        a = 6657;
        b = 13579.02468;
        complex1.clear();
        complex1.emplace_back(map<string, string>{});
        complex1.back().insert({ "Nana7mi's dog", "-JinShi- *&^#$#<>!" });
        complex1.emplace_back(map<string, string>{});
        complex1.back().insert({ "Nana7mi's dog", "-JinShi- *&^#$#<>!" });

        complex2.push_back(set<int>{114, 514, 114, 514});

        smartptr.reset(new int{ 1982 });
        pair_cb = std::make_pair('g', true);
    }
    bool operator==(Test rhs) const {
        return (a == rhs.a) && (b == rhs.b) && (complex1 == rhs.complex1) &&
            (complex2 == rhs.complex2) && (pair_cb == rhs.pair_cb);
    }
public:
    vector<map<string, string>> complex1;
    list<set<int>> complex2;
    pair<char, bool> pair_cb{'g', true};
    shared_ptr<int> smartptr;
    int a{ 1 };
    double b{ 2.0 };
};
int main()
{
    // ================================================================================basic types and nested containers
    string sa = "112233";
    string sb;
    BinSerde::serialize(sa, "test.data");
    BinSerde::deserialize(sb, "test.data");
    cout << sb << endl;
    assert(sa == sb);

    auto ia = 1;
    int ib;
    BinSerde::serialize(ia, "test.data");
    BinSerde::deserialize(ib, "test.data");
    cout << ib << endl;
    assert(ia == ib);

    pair<string, int> pa = make_pair("114", 514);
    pair<string, int> pb;
    BinSerde::serialize(pa, "test.data");
    BinSerde::deserialize(pb, "test.data");
    assert(pa == pb);
    cout << "(" << pb.first << "," << pb.second << ")" << endl;

    vector<vector<string>> vva{vector<string>{"Girimi", "Mahiru", "Nana7mi", "Azusa"},
        vector<string>{"21", "22", "23", "24"}};
    vector<vector<string>> vvb;
    BinSerde::serialize(vva, "test.data");
    BinSerde::deserialize(vvb, "test.data");
    assert(vva == vvb);
    cout << vvb[0][2] << endl;

    set<string> seta{"this", "is", "a", "container", "in", "a", "set"};
    set<string> setb;
    BinSerde::serialize(seta, "test.data");
    BinSerde::deserialize(setb, "test.data");
    assert(seta == setb);
    cout << setb.count("set") << endl;

    string b64s = "yyy's C++20 Lab!"; // base64 test
    string b64ed;
    cout << (b64ed = Serde::b64encode((unsigned char*)b64s.c_str(), b64s.size())) << endl;
    cout << Serde::b64decode((unsigned char*)b64ed.c_str(), b64ed.size()) << endl;

    // -----------------------------------------------------------------------------------------------------------------

    // ===========================================================================================custom classes, Binary
    // !NOTE: testing this custom class is equal to testing ALL the supported types acquired!
    // including NESTED containers, pairs, smart pointers...
    auto to_be_serialized = Test();
    to_be_serialized.fill_test_data(); // fill-in the initial value for tests
    if (!BinSerde::serialize(to_be_serialized, "test_custom.data"))
        cout << "failed:ser" << endl;
    auto recovered = Test();
    if (!BinSerde::deserialize(recovered, "test_custom.data"))
        cout << "failed:deser" << endl;
    assert(to_be_serialized == recovered);



    to_be_serialized.fill_test_data();
    if (!BinSerde::serialize(to_be_serialized, "test_b64.data", SERDE_B64))
        cout << "failed:ser" << endl;
    recovered = Test();
    if (!BinSerde::deserialize(recovered, "test_b64.data", SERDE_B64))
        cout << "failed:deser" << endl;
    assert(to_be_serialized == recovered);
    // -----------------------------------------------------------------------------------------------------------------

    // ===================================================================custom classes, XML(with (or not with) Base64)
    // XML: no Base64, internal API
    XMLDocument doc;
    doc.InsertEndChild(doc.NewElement("myname"));
    auto xml_test = Test();
    xml_test.fill_test_data();
    XmlSerde::serialize2xml(doc, doc.RootElement(), xml_test);

    auto xml_recovered = Test();
    XmlSerde::deserialize_from(doc.RootElement()->FirstChildElement(), xml_recovered);
    assert(xml_test == xml_recovered);
    // ====================================================================================XML: Base64, encapsulated API
    xml_test.fill_test_data();
    XmlSerde::serialize(xml_test, "xmlb64", "test_final.xml", SERDE_B64);
    xml_recovered = Test();
    XmlSerde::deserialize(xml_recovered, "xmlb64", "test_final.xml", SERDE_B64);
    assert(xml_test == xml_recovered);
    // -----------------------------------------------------------------------------------------------------------------
    cout<<"Tests Passed!!!"<<endl;
    return 0;
}