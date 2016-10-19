#ifndef SINLIB_REGEX_HPP
#define SINLIB_REGEX_HPP

#include <string>
#include <regex>
#include <vector>
#include <sstream>

namespace sinlib {

using std::regex;
using std::string;
using std::vector;
using std::istringstream;
using std::smatch;
using std::regex_search;

vector<string> regex_split(string input, const regex &rx, int max = 0);
bool regex_match_utf8(const string &input, const string &rx);

class regex_parser {
private:
	int _iter;
	string _input;
	string _next_input;
	smatch _match;
	
	template<typename T>
	void read_element(int el, T &val){
		istringstream param(_match[el].str());
		param >> val;
	}
	
	void read_element(int el, string &val){
		val = _match[el].str();
	}
public:
	regex_parser();
	regex_parser(const string &input);
	
	bool next(const regex &rx);
	
	bool valid();
	operator bool (){ return valid(); }
	
	void set_input(const string &input);

	inline string suffix(){ return _match.suffix(); }
	
	template<typename T>
	regex_parser &operator >> (T &val){
		if (valid()){
			++_iter;
			return read(_iter, val);
		}
		
		return *this;
	}
	
	template<typename T>
	regex_parser &read(int el, T &val){
		if (_iter >= 0 && el >= 0 && el < (int) _match.size()){
			read_element(el, val);
		}
		
		return *this;
	}
};

}

#endif

