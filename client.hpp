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
	set<RoomPtr> rooms;
	weak_ptr<Client> self;

	string name;
	int uid;
	bool _isGirl;
	string color;
public:
	time_t lastPacketTime;
	time_t lastMessageTime;
	int messageCounter;

	Client(Server *srv, shared_ptr<WSServerBase::Connection> conn){
		server = srv;
		connection = conn;
		uid = 0;
		lastMessageTime = time(nullptr);
		lastPacketTime = lastMessageTime;
		messageCounter = 0;
		_isGirl = false;
		color = "gray";
	}
	
	~Client(){
	
	}
	
	void setSelfPtr(weak_ptr<Client> wptr){ self = wptr; }
	ClientPtr getSelfPtr(){ return self.lock(); }

	inline bool isGirl(){ return _isGirl; }
	inline void setGirl(bool g){ _isGirl = g; }

	inline string getColor(){ return color; }
	inline void setColor(string clr){ color = clr; }

	inline string getName(){ return name; }
	inline void setName(const string &nm){ name = nm; }
	
	inline string getIP(){ return connection->remote_endpoint_address; }

	inline int getID(){ return uid; }
	inline void setID(int id){ uid = id; }
	
	inline Server *getServer(){ return server; }
	shared_ptr<WSServerBase::Connection> getConnection(){ return connection; }
	
	void onPacket(string pack);
	void onDisconnect();
	void onKick(RoomPtr room);
	
	MemberPtr joinRoom(RoomPtr room);
	void leaveRoom(RoomPtr room);

	RoomPtr getRoomByName(const string &name);
	inline const set<RoomPtr> &getConnectedRooms(){ return rooms; }

	void sendPacket(const Packet &);
	void sendRawData(const string &data);

	inline bool isAdmin(){
		return uid == 1 || uid == 2;
	}

	inline bool isGuest(){ return uid <= 0; }
};

#endif

