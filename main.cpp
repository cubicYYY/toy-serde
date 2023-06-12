#include "toyserde.hpp"
#include <map>
#include <vector>
using namespace std;
int main()
{
    char buf[128];
    int a=3;
    BinSerde::serialize2buf(&buf, a);
    BinSerde::deserialize_from(buf, a);
    cout<<a;
    return 0;
}