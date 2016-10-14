#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>
#include <jsoncpp/json/json.h>

class Config {
private:
public:
	Json::Value conf;

	Config(std::string file){
		loadFromFile(file, conf);
	}

	static void loadFromFile(std::string file, Json::Value &val);
	static void writeToFile(std::string file, const Json::Value &val);

	Json::Value operator [](std::string key){
		return conf[key];
	}
};

extern Config config;

#endif
