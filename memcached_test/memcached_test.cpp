#include "../memcached.hpp"
#include <iostream>

using namespace std;

string getval(const string &key, Memcache &cache){
	string val;
	cache.get(key, val);
	return val;
}

int main(int argc, char **argv){
	Memcache cache("localhost", 11211);
	cout << getval("key1", cache) << endl << getval("key2", cache) << endl << getval("key3", cache) << endl << getval("key4", cache) << endl;
	return 0;
}

