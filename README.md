# Introduction
A simple library to serialize multiple types of data. The design of the interface is inspired by Boost library.

# Requirements **(Important)**
Linux / WSL with g++ version >= **`10.3.0`**  
Compile options: `-fconcepts`, `-std=c++2a`  
Or you can use the `CMake` and `make` to build the project.  
CMakeLists.txt for CMake has been configured in the environment mentioned above.  

# Supported Types
+ Primary types:
	+ `std::is_arithmetic` (int, char, ...)
	+ `std::string` 
	+ STL containers (`std::pair`, `std::vector`, `std::list`, `std::set`, `std::map`)
	+ Smart pointers (`std::unique_ptr`, `std::shared_ptr`)   
  	NOTE: `std::weak_ptr` is not supported as intended. Obey the C++ standard rules!
+ Class / structs.
Usage of smart pointers are encouraged: more infos, less verbose, and less errors.

# Usage
## Single Variable
Including containers and smart pointers.
+ For datas with const size, just call serialize functions directly. 
+ For a pointer, please specify the size (in bytes) in args.

## Class/Struct Serialization
+ Use a template method `serializer` to enable the serialization / deserialization.
+ See examples followed for more details.

# Quick Start with Examples 
+ Single var, output as a binary file:
	```cpp
	string sa = "112233";
    string sb;
    BinSerde::serialize(sa, "test.data");
    BinSerde::deserialize(sb, "test.data");
    assert(sa == sb);
	```
+ Vector nested container, output as a binary file:
 	```cpp
	vector<vector<string>> vva{vector<string>{"Girimi", "Mahiru", "Nana7mi", "Azusa"},
        vector<string>{"21", "22", "23", "24"}};
    vector<vector<string>> vvb;
    BinSerde::serialize(vva, "test.data");
    BinSerde::deserialize(vvb, "test.data");
    assert(vva == vvb);
	```
+ Class with a nested container, output as a XML file, where strings are encoded in Base64:
 	```cpp
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
			complex1.back().insert({ "Nana7mi", "-JinShi- *&^#$#<>!" });

			complex2.push_back(set<int>{114, 514, 114, 514});

			smartptr.reset(new int{1982});
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
	Test to_be_serialized;
	to_be_serialized.fill_test_data();
	XmlSerde::serialize(to_be_serialized, "my_class", "test_b64.data", SERDE_B64);
	Test recovered;
	XmlSerde::deserialize(recovered, "my_class", "test_b64.data", SERDE_B64);
	assert(to_be_serialized==recovered);
	```

# Technical Details and Advanced Usage

## Some Thoughts
Will not separate impelmentations & definitions for templates.  
The reason is, these templates often instantiates in another file, and the linker will fail when searching in another file.  
To solve this, 
+ Manually instantiate them in header (Non-practical)  
+ Create a `.cpp` file including implementations, and include it at the last of a header file. (Weird)
+ Keep them in the same header file. Let it go.  
The simplest, the best (IMO).  
## Supported Basic Types
WYSIWYG.

## Array
We strongly encourage you to use STL container `std::array`. But if you really need it, you can use `SizedPair` to wrap it.(TODO)    
This also works for multidimensional arrays or dynamic sized array.   
Remember to cast the array to the most basic underlying type(e.g. `int[10][20][30]` -> `int *`, with size=`10*20*30=6000`).

## Classes
Class serialization needs can be of different scales: just a few integers of a tiny class, or a whole class including every items in member containers.  
For different needs, we provide following ways to satisfy them:  
+ Raw binary data shallow copy (platform dependent)
	+ May not be portable: type size varies between platforms!
	+ Just serialize it as an array: wrap it with SizedPair.

+ For deep serialization / custom needs:
	+ Using special template method `serde` to implement `serdeable` trait, and
	+ Only 1 method is enough: for both serialization & deserialization!
	+ Automatically called if find (thanks to SFINAE mechanism).
	+ Declaration is simple: you can use operator `&` to select fields you want to serialize.
	+ That is to say, it is a **declarative** way!
	+ You can use `NVP()` or `NAMED_NVP()` to wrap a value, which gives the info of var name to XML (de)serialization.  
	+ nested structs/types are supported!  
	(see examples for more details)

# Format(Only Binary Mode)
We have some meta datas to store, and multiple objects to track. So here's the format manual. Number in brackets indicates the size of the segment (in bytes).  
Global: `|Header|Field1|Field2|...|FieldN|`
+ `Header` part:  
`|MagicNum(4)|Version(4)|Flags(4)|DataSize(4)|`    

	The `MagicNum` segment : always equals to `0x21452505`. Used to check if the file is valid, **and indicates the platform endian property.**  
	The `version` segment allows us to read old data, when the format is changed in future versions.  
	The `Flags` segment stores the flags specified by users or automatically set.  

+ `Field` Part:  
Flexible length, can be of 2 types:
	+ type1(compound type):   
	`|ItemCounts(4)|Field1|Field2|...|FieldN|`  

		If and only if the Field stores a STL container, or a array introduced by a pointer.    
		**NOTE: You may noticed that the `Field` segment is defined recursively. So it supports nested structures!**

	+ type2(simple type):   
	`|ActuallData|`  

		The real data. Nothing more.

## Output
We use different flags to decide the representation of datas. You can use bitwise or `|` to specify multiple flags.
+ `S_XML` : to XML file if set, otherwise to binary
+ `S_B64` : Base64 encoding
NOTE:  
`S_XML` with `S_B64` will only apply Base64 encoding to  `std::string` (itself or sth. contains it);    
Otherwise the whole file will be stored in Base64 pure text format.

# References
+ Underlying principles: [Serialization and Unserialization, C++ FAQ](https://isocpp.org/wiki/faq/serialization)
+ Interfaces design is inspired by: [儲存 C++ 的類別資料：Boost Serialization（part 1）](https://viml.nchc.org.tw/archive_blog_760/)