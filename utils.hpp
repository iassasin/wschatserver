//
// Created by assasin on 03.05.19.
//

#ifndef WSSERVER_UTILS_HPP
#define WSSERVER_UTILS_HPP

#include <string>
#include "server.hpp"

inline std::string getRealClientIp(shared_ptr<WSServerBase::Connection> connection) {
	auto iphdr = connection->header.find("X-Real-IP");
	if (iphdr != connection->header.end()) {
		return iphdr->second;
	}

	return connection->remote_endpoint().address().to_string();
}

#endif //WSSERVER_UTILS_HPP
