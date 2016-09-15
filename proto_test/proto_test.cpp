#include "proto.hpp"

#include <iostream>

using namespace std;

int main(){
	ProtoObject obj;
	obj["1"] = "#let's test strings# Таки  дела## #";
	obj["2"] = 2;
	obj["3"] = 2.15;
	
	string sr = ProtoStream::serialize(obj);
	
	cout << sr << endl;
	
	sr = ProtoStream::serialize(ProtoStream::deserialize(sr));
	
	cout << sr << endl;
	
	return 0;
}

