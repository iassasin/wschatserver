#include "proto.hpp"
#include <stdio.h>

ProtoObject::operator int() const {
	switch (type){
		case Type::string:
		case Type::numeric:
			return (int) atof(value.c_str());
			
		case Type::boolean:
			return (int) (value != "0");
			
		case Type::array:
		case Type::null:
		case Type::dictionary:
			return 0;
	}
	return 0;
}

ProtoObject::operator double() const {
	switch (type){
		case Type::string:
		case Type::numeric:
			return atof(value.c_str());
			
		case Type::boolean:
			return (double) (int) (value != "0");
		
		case Type::array:
		case Type::null:
		case Type::dictionary:
			return 0.0;
	}
	return 0.0;
}

ProtoObject::operator bool() const {
	switch (type){
		case Type::string:
			return true;
			
		case Type::numeric:
			return atof(value.c_str()) != 0;
			
		case Type::boolean:
			return (double) (int) (value != "0");
		
		case Type::array:
		case Type::null:
		case Type::dictionary:
			return false;
	}
	return false;
}

ProtoObject::operator string() const {
	switch (type){
		case Type::numeric:			
		case Type::boolean:
		case Type::string:
			return value;
			
		case Type::array:
		case Type::null:
		case Type::dictionary:
			return "";
	}
	return "";
}

ProtoObject &ProtoObject::operator = (bool v){
	clearArrays();
	type = Type::boolean;
	value = v ? "1" : "0";
	return *this;
}

ProtoObject &ProtoObject::operator = (int v){
	clearArrays();
	type = Type::numeric;
	stringstream ss;
	ss << v;
	value = ss.str();
	return *this;
}

ProtoObject &ProtoObject::operator = (double v){
	clearArrays();
	type = Type::numeric;
	stringstream ss;
	ss << v;
	value = ss.str();
	return *this;
}

ProtoObject &ProtoObject::operator = (const string &v){
	clearArrays();
	type = Type::string;
	value = v;
	return *this;
}

ProtoObject &ProtoObject::operator = (const ProtoObject &v){
	clearArrays();
	value = v.value;
	type = v.type;
	array = v.array;
	dict = v.dict;
	return *this;
}

ProtoObject &ProtoObject::operator [](const string &v){
	array.clear();
	type = Type::dictionary;
	return dict[v];
}

ProtoObject &ProtoObject::operator [](int v){
	dict.clear();
	type = Type::array;
	if (v >= (int) array.size()){
		array.resize(v+1);
	}
	return array[v];
}

const ProtoObject &ProtoObject::operator [](const string &v) const {
	return dict.at(v);
}

const ProtoObject &ProtoObject::operator [](int v) const {
	return array[v];
}

/*
'u ' - null
'n123 ' - double(123)
'b1 ' - true
's# str## ' - " str#"
'a3 n1 n2.5 sstr ' - [1, 2.5, "str"]
'd3 login sadmin passwd spwd id n23 ' - {login: "admin", passwd: "pwd", id: 23}
*/

//{ null = 0, numeric, boolean, string, array, dictionary }
string ProtoStream::serialize(const ProtoObject &obj){
	int len;
	stringstream res;
	switch (obj.type){
		case ProtoObject::Type::null:
			return "u ";
			
		case ProtoObject::Type::numeric:
			return string("n") + (string) obj + " ";
		
		case ProtoObject::Type::boolean:
			return string("b") + ((bool) obj ? "1 " : "0 ");
		
		case ProtoObject::Type::string:
			return string("s") + escapeStr(obj) + " ";
		
		case ProtoObject::Type::array:
			len = obj.array.size();
			res << "a" << len << " ";
			for (ProtoObject o : obj.array){
				res << serialize(o);
			}
			return res.str();
			
		case ProtoObject::Type::dictionary:
			len = obj.dict.size();
			res << "d" << len << " ";
			for (auto o : obj.dict){
				res << escapeStr(o.first) + " ";
				res << serialize(o.second);
			}
			return res.str();
	}
	return "";
}

ProtoObject ProtoStream::deserialize(const string &str){
	if (str.empty()){
		return ProtoObject();
	}
	int pos = 0;
	return deserialize(str, pos);
}

ProtoObject ProtoStream::deserialize(const string &str, int &pos){
	ProtoObject obj;
	string tmp;
	int len;
	switch (str[pos]){
		case 'u':
			return obj;
			
		case 'n':
			obj.type = ProtoObject::Type::numeric;
			++pos;
			obj.value = readToSpace(str, pos, false);
			return obj;
			
		case 'b':
			obj.type = ProtoObject::Type::boolean;
			++pos;
			obj.value = readToSpace(str, pos, false);
			return obj;
		
		case 's':
			obj.type = ProtoObject::Type::string;
			++pos;
			obj.value = readToSpace(str, pos);
			return obj;
		
		case 'a':
			++pos;
			tmp = readToSpace(str, pos, false); 
			len = atol(tmp.c_str());
			for (int i = 0; i < len; ++i){
				obj[i] = deserialize(str, pos);
			}
			return obj;
		
		case 'd':
			++pos;
			tmp = readToSpace(str, pos, false); 
			len = atol(tmp.c_str());
			for (int i = 0; i < len; ++i){
				tmp = readToSpace(str, pos);
				obj[tmp] = deserialize(str, pos);
			}
			return obj;
	}
	return ProtoObject();
}

string ProtoStream::readToSpace(const string &str, int &pos, bool escapes){
	int len = str.size();
	int spos = pos;
	while (pos < len && str[pos] != ' '){
		if (escapes && str[pos] == '#'){
			if (pos+1 < len && (str[pos+1] == '#' || str[pos+1] == ' ')){
				++pos;
			}
		}
		++pos;
	}
	string res;
	if (pos > spos){
		res = string(str, spos, pos - spos);
		if (escapes){
			res = unescapeStr(res);
		}
	}
	if (pos < len){
		++pos;
	}
	
	return res;
}

string ProtoStream::escapeStr(const string &str){
	stringstream res;
	int spos = 0;
	int cur = 0;
	int len = str.size();
	while (cur < len){
		if (str[cur] == '#'){
			if (cur > spos){
				res << string(str, spos, cur - spos);
			}
			res << "##";
			spos = cur + 1;
		} else if (str[cur] == ' '){
			if (cur > spos){
				res << string(str, spos, cur - spos);
			}
			res << "# ";
			spos = cur + 1;
		}
		++cur;
	}
	if (cur > spos){
		res << string(str, spos, cur - spos);
	}
	return res.str();
}

string ProtoStream::unescapeStr(const string &str){
	stringstream res;
	int spos = 0;
	int cur = 0;
	int len = str.size();
	while (cur < len){
		if (str[cur] == '#'){
			if (cur+1 < len && (str[cur+1] == '#' || str[cur+1] == ' ')){
				if (cur > spos){
					res << string(str, spos, cur - spos);
				}
				++cur;
				res << str[cur];
				spos = cur + 1;
			}
		}
		++cur;
	}
	if (cur > spos){
		res << string(str, spos, cur - spos);
	}
	return res.str();
}

