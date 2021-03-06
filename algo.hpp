#ifndef ALGO_H_
#define ALGO_H_

#include <string>
#include <regex>
#include <vector>
#include <memory>

#include "regex/regex.hpp"

using namespace sinlib;

using std::string;
using std::vector;
using std::regex;
using std::unique_ptr;

#define REGEX_ANY_RUSSIAN_LOW "(а|б|в|г|д|е|ё|ж|з|и|й|к|л|м|н|о|п|р|с|т|у|ф|х|ц|ч|ш|щ|ъ|ы|ь|э|ю|я)"
#define REGEX_ANY_RUSSIAN_UP  "(А|Б|В|Г|Д|Е|Ё|Ж|З|И|Й|К|Л|М|Н|О|П|Р|С|Т|У|Ф|Х|Ц|Ч|Ш|Щ|Ъ|Ы|Ь|Э|Ю|Я)"
#define REGEX_ANY_RUSSIAN "(" REGEX_ANY_RUSSIAN_LOW "|" REGEX_ANY_RUSSIAN_UP ")"

string date(const string &format);
bool startsWith(const string &str, const string &needle);

template<typename T>
unique_ptr<T> as_unique(T *v) {
	return unique_ptr<T>(v);
}

void replaceInvalidUtf8(string &str, char replacement = '?');

#endif

