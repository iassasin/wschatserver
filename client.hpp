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
	shared_ptr<WSServerBase::Connection> connection;
	Server *server;
	string name;
	int uid;
	set<RoomPtr> rooms;
	weak_ptr<Client> self;
	bool _isGirl;
public:
	time_t lastMessageTime;
	int messageCounter;

	Client(Server *srv, shared_ptr<WSServerBase::Connection> conn){
		server = srv;
		connection = conn;
		uid = -1;
		lastMessageTime = time(nullptr);
		messageCounter = 0;
		_isGirl = false;
	}
	
	~Client(){
	
	}
	
	void setSelfPtr(weak_ptr<Client> wptr){ self = wptr; }
	ClientPtr getSelfPtr(){ return self.lock(); }

	bool isGirl(){ return _isGirl; }
	void setGirl(bool g){ _isGirl = g; }

	string getName(){ return name; }
	void setName(const string &nm){ name = nm; }
	
	string getIP(){ return connection->remote_endpoint_address; }

	int getID(){ return uid; }
	void setID(int id){ uid = id; }
	
	Server *getServer(){ return server; }
	shared_ptr<WSServerBase::Connection> getConnection(){ return connection; }
	
	void onPacket(string pack);
	void onDisconnect();
	void onKick(RoomPtr room);
	
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

