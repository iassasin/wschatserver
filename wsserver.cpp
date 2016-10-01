#include "server.hpp"

int main() {
	Server server(config["port"].asInt());
    server.start();
    
    return 0;
}

