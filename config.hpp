#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>
#include <jsoncpp/json/json.h>

class Config {
private:
	Json::Value conf;
public:
	Config(std::string file){
		loadFromFile(file);
	}

	void loadFromFile(std::string file);
	Json::Value operator [](std::string key){
		return conf[key];
	}
};

extern Config config;

#endif
