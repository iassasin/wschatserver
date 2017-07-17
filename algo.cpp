#include "algo.hpp"
#include <locale>

using namespace std;

string date(const string &format){
	time_t rawtime;
	struct tm *timeinfo;
	char buffer[format.size()*2];

	time (&rawtime);
	timeinfo = localtime(&rawtime);

	strftime (buffer, format.size()*2, format.c_str(), timeinfo);
	
	return buffer;
}

bool startsWith(const string &str, const string &needle){
	return string(str, 0, needle.size()) == needle;
}
