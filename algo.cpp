#include "algo.hpp"
#include <locale>
#include <utf8.h>

using namespace std;

string date(const string &format) {
	time_t rawtime;
	struct tm *timeinfo;
	char buffer[format.size()*2];

	time (&rawtime);
	timeinfo = localtime(&rawtime);

	strftime (buffer, format.size()*2, format.c_str(), timeinfo);
	
	return buffer;
}

bool startsWith(const string &str, const string &needle) {
	return string(str, 0, needle.size()) == needle;
}

void replaceInvalidUtf8(string &str, char replacement) {
	auto start = begin(str);
	auto send = end(str);

	while (start != send) {
		start = utf8::find_invalid(start, send);
		if (start != send)
			*start++ = replacement;
	}
}
