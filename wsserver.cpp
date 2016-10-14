#include "server.hpp"
#include <iostream>
#include <memory>
#include <exception>
#include <signal.h>

static std::shared_ptr<Server> server;

static void save_state(){
	try {
		Config::writeToFile("rooms.dat", server->serialize());
		cout << "Server state saved" << endl;
	} catch (exception &e){
		cerr << "Exception while loading old server state:" << endl
				<< e.what();
	}
}

static void sighandler(int signum){
	cout << "Received signal " << signum << endl;
	server->stop();
	save_state();
	exit(0);
}

int main() {
	if (signal(SIGTERM, sighandler) == SIG_ERR || signal(SIGINT, sighandler) == SIG_ERR){
		cerr << "Can't set signals. Aborting." << endl;
		return 1;
	}

//	Json::StyledWriter wr;
//	std::cout << wr.write(config.conf) << std::endl;

	server = std::make_shared<Server>(config["port"].asInt());

	try {
		Json::Value val;
		Config::loadFromFile("rooms.dat", val);
		server->deserialize(val);
		cout << "Last server state loaded" << endl;
	} catch (exception &e){
		cerr << "Exception while loading last server state:" << endl
				<< e.what();
	}


	server->start();

	save_state();

	return 0;
}

