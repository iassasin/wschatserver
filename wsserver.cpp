#include "server.hpp"
#include <iostream>
#include <memory>
#include <exception>
#include <signal.h>

#include "logger.hpp"

static std::shared_ptr<Server> server;

static void save_state(){
	try {
		Config::writeToFile("rooms.dat", server->serialize());
		Logger::info("Server state saved");
	} catch (exception &e){
		Logger::error("Exception while saving old server state:\n", e.what());
	}
}

static void sighandler(int signum){
	Logger::info("Received signal ", signum);
	server->stop();
	save_state();
	exit(signum != SIGTERM ? signum : 0);
}

int main() {
	if (signal(SIGTERM, sighandler) == SIG_ERR || signal(SIGINT, sighandler) == SIG_ERR || signal(SIGABRT, sighandler) == SIG_ERR){
		Logger::error("Can't set signals. Aborting.");
		return 1;
	}

	server = std::make_shared<Server>(config["port"].asInt());

	try {
		Json::Value val;
		Config::loadFromFile("rooms.dat", val);
		server->deserialize(val);
		Logger::info("Last server state loaded");
	} catch (exception &e){
		Logger::error("Exception while loading last server state:\n", e.what());
	}

	server->start();

	save_state();

	return 0;
}

