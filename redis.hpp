#ifndef MEMCACHED_H_
#define MEMCACHED_H_

#include <redox.hpp>
#include <string>
#include <sstream>
#include <ctime>
#include <memory>
#include <jsoncpp/json/json.h>

#include "logger.hpp"
#include "config.hpp"

using std::string;
using std::unique_ptr;
using redox::Redox;

class Redis {
private:
	static Redox redoxInstance;
	static bool connected;
	
	Redis(const Redis &) = delete;

	static bool isConnectionValid(){
		if (!connected){
			return false;
		}

		auto &cmd = redoxInstance.commandSync<string>({"PING"});
		bool ok = cmd.ok() && cmd.reply() == "PONG";
		cmd.free();
		return ok;
	}
public:
	Redis(){
		if (!isConnectionValid()){
			redoxInstance.connect(config["redis"]["host"].asCString(), config["redis"]["port"].asUInt());
			connected = true;
		}
	}

	template<typename T>
	bool get(const std::string &key, T &val){
		bool ok = false;

		auto &cmd = redoxInstance.commandSync<string>({"GET", key});

		if (cmd.status() == redox::Command<string>::OK_REPLY){
			string reply = cmd.reply();
			istringstream ss(reply);
			ss >> val;
			ok = true;
		}
		else if (cmd.status() == redox::Command<string>::NIL_REPLY){

		}

		cmd.free();
		return ok;
	}

	bool getJson(const std::string &key, Json::Value &val){
		string s;
		if (!get(key, s)){
			return false;
		}

		Json::Reader rd;
		return rd.parse(s, val);
	}
	
	template<typename T>
	bool set(const std::string &key, const T &value, time_t expiration = 0){
		if (expiration > 0){
			return redoxInstance.commandSync({"SETEX", std::to_string(expiration), std::to_string(value)});
		} else {
			return redoxInstance.set(key, std::to_string(value));
		}
	}

	bool set(const std::string &key, const string &value, time_t expiration = 0){
		if (expiration > 0){
			return redoxInstance.commandSync({"SETEX", std::to_string(expiration), value});
		} else {
			return redoxInstance.set(key, value);
		}
	}

	bool setJson(const std::string &key, const Json::Value &value, time_t expiration = 0){
		Json::FastWriter writer;
		return set(key, writer.write(value), expiration);
	}
	
};

#endif

