#ifndef PROTO_H_
#define PROTO_H_

#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>

using std::string;
using std::stringstream;
using std::map;
using std::vector;

class ProtoObject {
public:
	enum class Type : int { null = 0, numeric, boolean, string, array, dictionary };
private:
	void clearArrays(){
		dict.clear();
		array.clear();
	}
public:
	Type type;
	string value;
	map<string, ProtoObject> dict;
	vector<ProtoObject> array;

	ProtoObject(){
		type = Type::null;
	}
	
	ProtoObject(const ProtoObject &o){
		*this = o;
	}
	
	void null(){
		type = Type::null;
		clearArrays();
	}
	
	void remove(int idx){
		array.erase(array.begin() + idx);
	}
	
	void remove(const string &k){
		dict.erase(k);
	}
	
	void remove(const char *k){
		remove(string(k));
	}
	
	operator int() const;
	operator double() const;
	operator bool() const;
	operator string() const;
	
	ProtoObject &operator = (bool);
	ProtoObject &operator = (int);
	ProtoObject &operator = (double);
	ProtoObject &operator = (const string &);
	ProtoObject &operator = (const char *s){ return *this = string(s); }
	ProtoObject &operator = (const ProtoObject &);
	
	ProtoObject &operator [](const string &);
	ProtoObject &operator [](const char *s){ return (*this)[string(s)]; };
	ProtoObject &operator [](int);
	
	const ProtoObject &operator [](const string &) const;
	const ProtoObject &operator [](const char *s) const { return (*this)[string(s)]; };
	const ProtoObject &operator [](int) const;
};

class ProtoStream {
private:
	static string escapeStr(const string &);
	static string unescapeStr(const string &);
	static string readToSpace(const string &, int &, bool = true);
	static ProtoObject deserialize(const string &, int &);
public:
	static string serialize(const ProtoObject &);
	static ProtoObject deserialize(const string &);
};

#endif

