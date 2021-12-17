//
// Created by assasin on 03.05.19.
//

#ifndef WSSERVER_UTILS_HPP
#define WSSERVER_UTILS_HPP

#include <string>
#include <random>
#include "server_fwd.hpp"

inline std::string getRealClientIp(ConnectionPtr connection) {
	auto iphdr = connection->header.find("X-Real-IP");
	if (iphdr != connection->header.end()) {
		return iphdr->second;
	}

	return connection->remote_endpoint().address().to_string();
}

class Keygen {
public:
	Keygen() {
		std::random_device dev;
		generator.seed(dev()); // may throw on very specific OS/devices
		distribution = std::uniform_int_distribution<uint64_t>(0, alphabet.size() - 1);
	}

	std::string generate(size_t len) {
		std::string result;
		result.reserve(len);

		for (size_t i = 0; i < len; ++i) {
			result += alphabet[distribution(generator)];
		}

		return result;
	}
private:
	std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

	std::mt19937_64 generator;
	std::uniform_int_distribution<uint64_t> distribution;
};

#endif //WSSERVER_UTILS_HPP
