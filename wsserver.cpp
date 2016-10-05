#include "server.hpp"
#include <iostream>

int main() {
//	Json::StyledWriter wr;
//	std::cout << wr.write(config.conf) << std::endl;

	Server server(config["port"].asInt());

	auto room = server.createRoom("#chat");
	room->setOwner(2);
	room->onCreate();

    server.start();
    
    return 0;
}

