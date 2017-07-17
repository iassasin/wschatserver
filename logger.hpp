//
// Created by assasin on 17.07.17.
//

#ifndef BUILD_LOGGER_HPP
#define BUILD_LOGGER_HPP

#include <iostream>
#include <string>
#include <ctime>

#include "algo.hpp"

class Logger {
private:
	Logger(){}
	~Logger(){}

	template<typename T, typename ...Args>
	static void doLog(std::ostream &s, const T &arg, const Args &...args){
		s << arg;
		doLog(s, args...);
	}

	template<typename T>
	static void doLog(std::ostream &s, const T &arg){
		s << arg << std::endl;
	}

	template<typename ...Args>
	static void logErr(const Args &...args){
		std::ostream *s = &std::cerr;
//		*s << date("[%Y-%m-%d %H:%M:%S] ");
		doLog(*s, args...);
	}
public:
	template<typename ...Args>
	static void log(const Args &...args){
		std::ostream *s = &std::cout;
//		*s << date("[%Y-%m-%d %H:%M:%S] ");
		doLog(*s, args...);
	}

	template<typename ...Args>
	static void error(const Args &...args){
		logErr("[ERROR] ", args...);
	}

	template<typename ...Args>
	static void warn(const Args &...args){
		log("[WARNING] ", args...);
	}

	template<typename ...Args>
	static void info(const Args &...args){
		log("[INFO] ", args...);
	}

	template<typename ...Args>
	static void debug(const Args &...args){
		log("[DEBUG] ", args...);
	}
};

#endif //BUILD_LOGGER_HPP
