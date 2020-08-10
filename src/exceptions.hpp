//
// Created by assasin on 10.08.2020.
//

#ifndef WSSERVER_EXCEPTIONS_HPP
#define WSSERVER_EXCEPTIONS_HPP

#include <exception>
#include <stdexcept>

class ChatException : std::runtime_error {
public:
	ChatException(const std::string &msg) : std::runtime_error(msg) {}
};

class BannedByIPException : ChatException {
public:
	BannedByIPException(const std::string &msg) : ChatException(msg) {}
};

class BannedByNickException : ChatException {
public:
	BannedByNickException(const std::string &msg) : ChatException(msg) {}
};

class BannedByIDException : ChatException {
public:
	BannedByIDException(const std::string &msg) : ChatException(msg) {}
};

#endif //WSSERVER_EXCEPTIONS_HPP
