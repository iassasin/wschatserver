#include "algo.hpp"
#include <locale>

using namespace std;

vector<string> regex_split(string input, const regex &rx, int max){
	smatch ma;
	vector<string> res;
	
	if (max > 0){
		while (max > 1 && regex_search(input, ma, rx)){
			res.push_back(ma.prefix().str());
			input = ma.suffix().str();
			--max;
		}
	} else {
		while (regex_search(input, ma, rx)){
			res.push_back(ma.prefix().str());
			input = ma.suffix().str();
		}
	}
	
	res.push_back(input);
	
	return res;
}

bool regex_match_utf8(const string &input, const string &rx){
	locale old;
	locale::global(std::locale("en_US.UTF-8"));
	
	regex pattern(rx, regex_constants::extended);
	bool result = regex_match(input, pattern);

	locale::global(old);

	return result;
}

string date(const string &format){
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[format.size()*2];

	time (&rawtime);
	timeinfo = localtime (&rawtime);

	strftime (buffer, format.size()*2, format.c_str(), timeinfo);
	
	return buffer;
}

bool startsWith(const string &str, const string &needle){
	return string(str, 0, needle.size()) == needle;
}

