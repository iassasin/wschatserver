#include <vector>
#include <map>
#include <time.h>
#include <regex>

#include "algo.hpp"
#include "client.hpp"
#include "server.hpp"
#include "packets.hpp"

Server::Server(int port)
	: server(port, 2), cache("localhost", 11211)
{
	auto& chat = server.endpoint["^/chat/?$"];
	
	chat.onmessage = [&](auto connection, auto message) {
	    stringstream data_ss;
	    message->data >> data_ss.rdbuf();
	    clients[connection]->onPacket(data_ss.str());
	};
	
	chat.onopen = [&, this](auto connection) {
	    cout << date("[%H:%M:%S] ") << "Server: Opened connection " << (size_t)connection.get() << endl;
	    shared_ptr<Client> cli(new Client(this, connection));
	    clients[connection] = cli;
	    
	    for (auto &v : pack_history){
	    	this->sendRawData(connection, v);
	    }
	    
	    //TODO: client init function
	    PacketSystem msg("Перед началом общения укажите свой ник: /nick MyNick");
	    this->sendPacket(connection, msg);
	};
	
	chat.onclose = [&](auto connection, int status, const string& reason) {
	    cout << date("[%H:%M:%S] ") << "Server: Closed connection " << (size_t)connection.get() << " with status code " << status << endl;
	    clients[connection]->onDisconnect();
	    clients.erase(connection);
	};
	
	chat.onerror = [&](auto connection, const boost::system::error_code& ec) {
		cout << date("[%H:%M:%S] ") << "Server: Error in connection " << (size_t)connection.get() << ". " <<
				"Error: " << ec << ", error message: " << ec.message() << endl;
		clients[connection]->onDisconnect();
		clients.erase(connection);
	};
	
}

void Server::start(){
	server.start();
}

void Server::addToHistory(const string &spack){
	pack_history.push_back(spack);
	if (pack_history.size() > 50){
		pack_history.pop_front();
	}
}

void Server::sendRawData(shared_ptr<SocketServerBase<WS>::Connection> conn, const string &rdata){
	stringstream response_ss;
	response_ss << rdata;
	server.send(conn, response_ss);
}

void Server::sendPacket(shared_ptr<SocketServerBase<WS>::Connection> conn, const Packet &pack){
	Json::FastWriter wr;
	stringstream response_ss;
	response_ss << wr.write(pack.serialize());
	server.send(conn, response_ss);
}

void Server::sendPacketToAll(const Packet &pack){
	Json::FastWriter wr;
	stringstream response_ss;
	string spack = wr.write(pack.serialize());
	if (pack.type == Packet::Type::message)
		addToHistory(spack);
	response_ss << spack;
	for (auto conn : server.get_connections()){
		response_ss.seekg(0);
		server.send(conn, response_ss);
	}
}

shared_ptr<Client> Server::getClientByName(string name){
	for (auto clip : clients){
		auto cli = clip.second;
		if (!cli->getName().empty() && cli->getName() == name)
			return cli; 
	}
	return shared_ptr<Client>();
}

shared_ptr<Client> Server::getClientByID(int uid){
	for (auto clip : clients){
		auto cli = clip.second;
		if (cli->getID() > 0 && cli->getID() == uid)
			return cli; 
	}
	return shared_ptr<Client>();
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

