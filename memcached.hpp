#ifndef MEMCACHED_H_
#define MEMCACHED_H_

#include <libmemcached/memcached.h>
#include <string>
#include <sstream>
#include <ctime>
#include <memory>

#include "logger.hpp"
#include "config.hpp"

using std::string;
using std::unique_ptr;

class Memcache {
private:
	struct memcached_st_my {
		memcached_st *mci;

		memcached_st_my(){
			mci = memcached(nullptr, 0);
			if (mci){
				memcached_server_add(mci, config["memcache"]["host"].asCString(), config["memcache"]["port"].asUInt());
			}
		}

		~memcached_st_my(){
			memcached_free(mci);
		}

		bool valid(){
			return true;
		}
	};

	static unique_ptr<memcached_st_my> mci;
	
	Memcache(const Memcache &);
public:
	Memcache(){
		if (!mci || !mci->valid()){
			mci.reset(new memcached_st_my());
		}
	}

	bool error() const {
		return memcached_failed(memcached_last_error(mci->mci));
	}
	
	bool addServer(const std::string &server_name, in_port_t port){
		return memcached_success(memcached_server_add(mci->mci, server_name.c_str(), port));
	}

	template<typename T>
	bool get(const std::string &key, T &val){
		uint32_t flags = 0;
		memcached_return_t rc;
		size_t value_length = 0;

		char *value = memcached_get(mci->mci, key.c_str(), key.length(), &value_length, &flags, &rc);
		if (value != nullptr){
			val = *(T *) value;
			free(value);
			return true;
		}

		return false;
	}

	bool get(const std::string &key, string &val){
		uint32_t flags = 0;
		memcached_return_t rc;
		size_t value_length = 0;

		char *value = memcached_get(mci->mci, key.c_str(), key.length(), &value_length, &flags, &rc);
		if (value != nullptr){
			val = string(value, value_length);
			free(value);
			return true;
		}

		return false;
	}

	template<typename T>
	bool getPhp(const std::string &key, T &val){
		string value;
		if (get(key, value)){
			istringstream ss(value);
			ss >> val;
			return true;
		}

		return false;
	}
	
	template<typename T>
	bool set(const std::string &key, const T &value, time_t expiration = 0, uint32_t flags = 0){
		memcached_return_t rc = memcached_set(mci->mci, key.c_str(), key.length(),
				(const char *) &value, sizeof(T), expiration, flags);
		return memcached_success(rc);
	}

	template<typename T>
	bool setPhp(const std::string &key, const T &value, time_t expiration = 0, uint32_t flags = 0){
		return set(key, std::to_string(value), expiration, flags);
	}

	bool set(const std::string &key, const string &value, time_t expiration = 0, uint32_t flags = 0){
		memcached_return_t rc = memcached_set(mci->mci, key.c_str(), key.length(),
				value.c_str(), value.length(), expiration, flags);
		return memcached_success(rc);
	}

	bool set(const std::string &key, const char *value, time_t expiration = 0, uint32_t flags = 0){
		memcached_return_t rc = memcached_set(mci->mci, key.c_str(), key.length(),
				value, strlen(value), expiration, flags);
		return memcached_success(rc);
	}
	
};

#endif

