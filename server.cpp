#include <vector>
#include <map>
#include <time.h>
#include <regex>

#include "algo.hpp"
#include "client.hpp"
#include "server.hpp"
#include "packets.hpp"

Server::Server(int port)
	: server(port, 1, config["ssl"]["certificate"].asString(), config["ssl"]["private_key"].asString())
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

	server.runWithInterval(pingInterval, [&]{
		time_t cur = time(nullptr);
		vector<ClientPtr> toKick;

		for (auto c : clients){
			auto &cli = c.second;

			if (cur - cli->lastPacketTime > connectTimeout){
				toKick.push_back(cli);
			}
			else if (cur - cli->lastPacketTime > pingTimeout){
				//cout << date("[%H:%M:%S] ") << "Server: Sending ping to " << cli->getName() << " [" << cli->getIP() << "]" << endl;
				cli->sendPacket(PacketPing());
			}
		}

		for (auto cli : toKick){
			kick(cli);
			cout << date("[%H:%M:%S] ") << "Server: Kicked by inactive: " << cli->getName() << " [" << cli->getIP() << "]" << endl;
		}
	});
}

void Server::start(){
	server.start();
}

void Server::stop(){
	server.stop();
}

Json::Value Server::serialize(){
	Json::Value val;

	auto &sr = val["rooms"];
	for (RoomPtr r : rooms){
		sr.append(r->serialize());
	}

	return val;
}

void Server::deserialize(const Json::Value &val){
	rooms.clear();
	for (auto &v : val["rooms"]){
		RoomPtr rm = make_shared<Room>(this);
		rm->setSelfPtr(rm);
		rm->deserialize(v);
		rooms.insert(rm);
	}
}

void Server::sendRawData(shared_ptr<WSServerBase::Connection> conn, const string &rdata){
	auto response_ss = make_shared<SendStream>();
	*response_ss << rdata;
	server.send(conn, response_ss);
}

void Server::sendPacket(shared_ptr<WSServerBase::Connection> conn, const Packet &pack){
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

vector<ClientPtr> Server::getClients(){
	vector<ClientPtr> res;
	for (auto clip : clients){
		auto &cli = clip.second;
		res.push_back(cli);
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
		return nullptr;

	rm = make_shared<Room>(this);
	rm->setName(name);
	rm->setSelfPtr(rm);
	rooms.insert(rm);
	rm->onCreate();

	return rm;
}

bool Server::removeRoom(string name){
	auto rm = getRoomByName(name);
	if (rm){
		bool res = rooms.erase(rm) > 0;
		if (res){
			rm->onDestroy();
		}
		return res;
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

