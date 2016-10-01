#include "config.hpp"
#include <iostream>
#include <fstream>

Config config("wsserver.conf");

void Config::loadFromFile(std::string file){
	Json::Reader rd;
	std::ifstream ifs(file);
	conf.clear();
	rd.parse(ifs, conf);
}
