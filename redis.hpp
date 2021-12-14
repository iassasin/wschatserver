#ifndef MEMCACHED_H_
#define MEMCACHED_H_

#include <string>
#include <sstream>
#include <ctime>
#include <memory>
#include <jsoncpp/json/json.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <redisclient/redissyncclient.h>

#include "logger.hpp"
#include "config.hpp"

using std::string;
using std::unique_ptr;

class Redis {
private:
	static boost::asio::io_context io;
	static redisclient::RedisSyncClient redisInstance;
	
	Redis(const Redis &) = delete;

	static bool isConnectionValid() {
		using namespace std::string_literals;

		if (!redisInstance.isConnected()) {
			return false;
		}

		try {
			auto cmd = redisInstance.command("PING", {});
			return cmd.isOk() && cmd.toString() == "PONG";
		} catch (std::exception &e) {
			Logger::error("Error in redis check connection: "s + e.what());
		}

		return false;
	}
public:
	Redis() {
		if (!isConnectionValid()) {
			redisInstance.disconnect();

			boost::asio::ip::tcp::resolver resolver(io);
			boost::asio::ip::tcp::resolver::query query(
					config["redis"]["host"].asCString(),
					std::to_string(config["redis"]["port"].asUInt())
			);
			auto ep = resolver.resolve(query)->endpoint();
			redisInstance.connect(ep);

			Logger::info("Connected to redis");
		}
	}

	template<typename T>
	bool get(const std::string &key, T &val) {
		auto cmd = redisInstance.command("GET", {key});

		if (cmd.isNull()) {

		}
		else if (cmd.isOk()) {
			string reply = cmd.toString();
			istringstream ss(reply);
			ss >> val;
			return true;
		}

		return false;
	}

	bool getJson(const std::string &key, Json::Value &val) {
		string s;
		if (!get(key, s)) {
			return false;
		}

		Json::Reader rd;
		return rd.parse(s, val);
	}
	
	template<typename T>
	bool set(const std::string &key, const T &value, time_t expiration = 0) {
		if (expiration > 0) {
			return redisInstance.command("SETEX", {std::to_string(expiration), std::to_string(value)}).isOk();
		} else {
			return redisInstance.command("SET", {key, std::to_string(value)}).isOk();
		}
	}

	bool set(const std::string &key, const string &value, time_t expiration = 0) {
		if (expiration > 0) {
			return redisInstance.command("SETEX", {std::to_string(expiration), value}).isOk();
		} else {
			return redisInstance.command("SET", {key, value}).isOk();
		}
	}

	bool setJson(const std::string &key, const Json::Value &value, time_t expiration = 0) {
		Json::FastWriter writer;
		return set(key, writer.write(value), expiration);
	}
	
};

#endif

