#include "regex.hpp"
#include <locale>

namespace sinlib {

using std::locale;
using std::regex_match;

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
	locale::global(locale("en_US.UTF-8"));
	
	regex pattern(rx, std::regex_constants::extended);
	bool result = regex_match(input, pattern);

	locale::global(old);

	return result;
}

// class regex_parser

regex_parser::regex_parser(){
	_iter = -1;
}

regex_parser::regex_parser(const string &input){
	_next_input = input;
	_iter = -1;
}

bool regex_parser::valid(){
	return _iter >= 0 && _iter < (int) _match.size();
}

void regex_parser::set_input(const string &input){
	_next_input = input;
	_iter = -1;
}

bool regex_parser::next(const regex &rx){
	if (_next_input.empty()){
		return false;
	}
	_input = _next_input;
	if (regex_search(_input, _match, rx)){
		_iter = 0;
		_next_input = _match.suffix().str();
		return true;
	} else {
		_iter = -1;
		return false;
	}
}

}

