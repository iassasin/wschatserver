#include "config.hpp"
#include <iostream>
#include <fstream>

Config config("wsserver.conf.json");

void Config::loadFromFile(std::string file, Json::Value &val) {
	Json::Reader rd;
	std::ifstream ifs(file);
	val.clear();
	rd.parse(ifs, val);
}

void Config::writeToFile(std::string file, const Json::Value &val) {
	Json::StyledStreamWriter wr;
	std::ofstream ofs(file);
	wr.write(ofs, val);
}
