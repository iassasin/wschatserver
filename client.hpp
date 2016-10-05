#ifndef CLIENT_H_
#define CLIENT_H_

#include <memory>

class Client;
using ClientPtr = std::shared_ptr<Client>;

#include "server_ws.hpp"
#include "packet.hpp"
#include "server.hpp"
#include "rooms.hpp"

using namespace std;
using namespace SimpleWeb;

class Client {
private:
	shared_ptr<SocketServerBase<WS>::Connection> connection;
	Server *server;
	string name;
	int uid;
	set<RoomPtr> rooms;
	weak_ptr<Client> self;
public:
	Client(Server *srv, shared_ptr<SocketServerBase<WS>::Connection> conn){
		server = srv;
		connection = conn;
		uid = -1;
	}
	
	~Client(){
	
	}
	
	void setSelfPtr(weak_ptr<Client> wptr){ self = wptr; }
	ClientPtr getSelfPtr(){ return self.lock(); }

	string getName(){ return name; }
	void setName(const string &nm){ name = nm; }
	
	string getIP(){ return connection->remote_endpoint_address.to_string(); }

	int getID(){ return uid; }
	void setID(int id){ uid = id; }
	
	Server *getServer(){ return server; }
	shared_ptr<SocketServerBase<WS>::Connection> getConnection(){ return connection; }
	
	void onPacket(string pack);
	void onDisconnect();
	
	MemberPtr joinRoom(RoomPtr room);
	void leaveRoom(RoomPtr room);

	RoomPtr getRoomByName(const string &name);

	void sendPacket(const Packet &);
	void sendRawData(const string &data);

	bool isAdmin(){
		return uid == 1 || uid == 2;
	}
};

#endif

