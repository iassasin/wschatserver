#include "server.hpp"
#include <iostream>
#include <memory>
#include <exception>
#include <csignal>

#include "logger.hpp"

static std::shared_ptr<Server> server;

static void save_state() {
	try {
		Config::writeToFile("rooms.dat", server->serialize());
		Logger::info("Server state saved");
	} catch (exception &e) {
		Logger::error("Exception while saving old server state:\n", e.what());
	}
}

static void sigShutdown(int signum) {
	Logger::info("Received signal ", signum);
	server->stop();
	save_state();
	exit(signum != SIGTERM ? signum : 0);
}

static void sigSave(int signum) {
	Logger::info("Received signal ", signum);
	save_state();
}

int main() {
	struct sigaction action{};

	action.sa_handler = sigShutdown;
	for (int sig : { SIGTERM, SIGINT, SIGABRT }) {
		if (sigaction(sig, &action, nullptr) != 0) {
			Logger::error("Can't set signal ", sig, ". Aborting.");
			return 1;
		}
	}

	if (signal(SIGUSR1, sigSave) == SIG_ERR) {
		Logger::error("Can't set signal ", SIGUSR1, ". Aborting.");
		return 1;
	}

	server = std::make_shared<Server>(config);

	try {
		Json::Value val;
		Config::loadFromFile("rooms.dat", val);
		server->deserialize(val);
		Logger::info("Last server state loaded");
	} catch (exception &e) {
		Logger::error("Exception while loading last server state:\n", e.what());
	}

	server->start();

	save_state();

	return 0;
}

