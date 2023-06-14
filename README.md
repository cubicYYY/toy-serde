# Introduction
A simple library to serialize multiple types of data. The design of the interface is inspired by Boost library.

# Requirements **(Important)**
g++ version: >= **`10.3.0`**  
Compile options: `-fconcepts`, `-std=c++2a`  
Or you can use the `CMake` and `make` to build the project.  

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
	some code here
	```
+ Vector container, output as a binary file:
 	```cpp
	some code here
	```
+ Struct with a nested container, output as a XML file, where strings are encoded in Base64:
 	```cpp
	some code here
	```

# Technical Details about Class/Struct Serialization
Class serialization needs can be of different scales: just a few integers of a tiny class, or a whole class including every items in member containers.  
For different needs, we provide following ways to satisfy them:  
+ Fallback/default solution: raw binary data shallow copy (platform dependent)
	+ May not be portable: type size varies between platforms!
	+ little-endian and big-endian is automatically determined! (TODO)

+ For deep serialization / custom needs:
	+ using special template method `serde` to implement `serdeable` trait, and
	+ only 1 method is enough: for both serialization & deserialization!
	+ automatically called if find (thanks to SFINAE mechanism).
	+ declaration is simple: you can use operator `&` to select fields you want to serialize.
	+ nested structs/types are supported!  
	(see examples for more details)

# Format(Binary)
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