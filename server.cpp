#include <vector>
#include <map>
#include <time.h>
#include <regex>

#include "algo.hpp"
#include "client.hpp"
#include "server.hpp"
#include "packets.hpp"

Server::Server(int port)
	: server(port, 1)
{
	cout << "Started wsserver at port " << port << endl;

	auto& chat = server.endpoint["^/chat/?$"];
	
	chat.onmessage = [&](auto connection, auto message) {
	    if (clients.find(connection) != clients.end()){
	    	clients[connection]->onPacket(message->string());
	    }
	};
	
	chat.onopen = [&, this](auto connection) {
	    cout << date("[%H:%M:%S] ") << "Server: Opened connection #" << (size_t)connection.get()
	    		<< " (" << connection->remote_endpoint_address << ")" << endl;

	    ClientPtr cli = make_shared<Client>(this, connection);
	    cli->setSelfPtr(cli);
	    clients[connection] = cli;
	};
	
	chat.onclose = [&](auto connection, int status, const string& reason) {
	    cout << date("[%H:%M:%S] ") << "Server: Closed connection #" << (size_t)connection.get()
	    		<< " (" << connection->remote_endpoint_address << ")" << " with status code " << status << endl;

	    if (clients.find(connection) != clients.end()){
			clients[connection]->onDisconnect();
			clients.erase(connection);
	    }
	};
	
	chat.onerror = [&](auto connection, const boost::system::error_code& ec) {
		cout << date("[%H:%M:%S] ") << "Server: Error in connection #" << (size_t)connection.get()
				<< " (" << connection->remote_endpoint_address << "). "
				<< "Error: " << ec << ", error message: " << ec.message() << endl;

		if (clients.find(connection) != clients.end()){
			clients[connection]->onDisconnect();
			clients.erase(connection);
		}
	};
	
}

void Server::start(){
	server.start();
}

void Server::sendRawData(shared_ptr<SocketServerBase<WS>::Connection> conn, const string &rdata){
	auto response_ss = make_shared<SendStream>();
	*response_ss << rdata;
	server.send(conn, response_ss);
}

void Server::sendPacket(shared_ptr<SocketServerBase<WS>::Connection> conn, const Packet &pack){
	Json::FastWriter wr;
	auto response_ss = make_shared<SendStream>();
	*response_ss << wr.write(pack.serialize());
	server.send(conn, response_ss);
}

void Server::sendPacketToAll(const Packet &pack){
	Json::FastWriter wr;
	auto response_ss = make_shared<SendStream>();
	string spack = wr.write(pack.serialize());

	*response_ss << spack;
	for (auto conn : server.get_connections()){
		//response_ss->seekg(0);
		server.send(conn, response_ss);
	}
}

ClientPtr Server::getClientByName(string name){
	for (auto clip : clients){
		auto cli = clip.second;
		if (!cli->getName().empty() && cli->getName() == name)
			return cli; 
	}
	return ClientPtr();
}

ClientPtr Server::getClientByID(int uid){
	for (auto clip : clients){
		auto cli = clip.second;
		if (cli->getID() > 0 && cli->getID() == uid)
			return cli; 
	}
	return ClientPtr();
}

vector<string> Server::getClients(){
	vector<string> res;
	for (auto clip : clients){
		auto cli = clip.second;
		if (!cli->getName().empty()){
			res.push_back(cli->getName());
		}
	}
	return res;
}

void Server::kick(ClientPtr client){
	auto conn = client->getConnection();
	clients.erase(conn);
	client->onDisconnect();
	server.send_close(conn, 0);
}

RoomPtr Server::createRoom(string name){
	auto rm = getRoomByName(name);
	if (rm)
		return rm;

	rm = make_shared<Room>(this);
	rm->setName(name);
	rm->setSelfPtr(rm);
	rooms.insert(rm);
	return rm;
}

bool Server::removeRoom(string name){
	auto rm = getRoomByName(name);
	if (rm){
		return rooms.erase(rm) > 0;
	}

	return false;
}

RoomPtr Server::getRoomByName(string name){
	for (RoomPtr room : rooms){
		if (room->getName() == name){
			return room;
		}
	}

	return nullptr;
}

