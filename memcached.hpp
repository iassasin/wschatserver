#ifndef MEMCACHED_H_
#define MEMCACHED_H_

#include <libmemcached/memcached.h>
#include <string>
#include <string.h>
#include <time.h>

using std::string;

class Memcache {
private:
	memcached_st *mci;
	
	Memcache();
	Memcache(const Memcache &);
public:
	Memcache(const std::string &hostname, in_port_t port){
		mci = memcached(NULL, 0);
		if (mci){
			memcached_server_add(mci, hostname.c_str(), port);
		}
	}
	
	~Memcache(){
		memcached_free(mci);
	}
	
	bool error() const {
		return memcached_failed(memcached_last_error(mci));
	}
	
	bool addServer(const std::string &server_name, in_port_t port){
		return memcached_success(memcached_server_add(mci, server_name.c_str(), port));
	}

	template<typename T>
	bool get(const std::string &key, T &val){
		uint32_t flags = 0;
		memcached_return_t rc;
		size_t value_length = 0;

		char *value = memcached_get(mci, key.c_str(), key.length(), &value_length, &flags, &rc);
		if (value != NULL){
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

		char *value = memcached_get(mci, key.c_str(), key.length(), &value_length, &flags, &rc);
		if (value != NULL){
			val = string(value, value_length);
			free(value);
			return true;
		}

		return false;
	}
	
	template<typename T>
	bool set(const std::string &key, const T &value, time_t expiration = 0, uint32_t flags = 0){
		memcached_return_t rc = memcached_set(mci, key.c_str(), key.length(),
				&value, sizeof(T), expiration, flags);
		return memcached_success(rc);
	}

	bool set(const std::string &key, const string &value, time_t expiration = 0, uint32_t flags = 0){
		memcached_return_t rc = memcached_set(mci, key.c_str(), key.length(),
				value.c_str(), value.length(), expiration, flags);
		return memcached_success(rc);
	}

	bool set(const std::string &key, const char *value, time_t expiration = 0, uint32_t flags = 0){
		memcached_return_t rc = memcached_set(mci, key.c_str(), key.length(),
				value, strlen(value), expiration, flags);
		return memcached_success(rc);
	}
	
};

#endif

