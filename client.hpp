#ifndef CLIENT_H_
#define CLIENT_H_

#include "client_fwd.hpp"
#include "packet.hpp"
#include "server_fwd.hpp"
#include "rooms_fwd.hpp"

using namespace std;

class Client {
private:
	ConnectionPtr connection;
	Server *server;
	set<RoomPtr> rooms;
	weak_ptr<Client> self;

	string name;
	uint uid;
	bool _isGirl;
	string color;
	string lastClientIP;
	string token;
public:
	time_t lastPacketTime;
	time_t lastMessageTime;
	int messageCounter;

	Client(Server *srv, string token);
	~Client();
	
	void setSelfPtr(weak_ptr<Client> wptr) { self = wptr; }
	ClientPtr getSelfPtr() { return self.lock(); }

	bool isGirl() { return _isGirl; }
	void setGirl(bool g) { _isGirl = g; }

	string getColor() { return color; }
	void setColor(string clr) { color = clr; }

	string getName() { return name; }
	void setName(const string &nm) { name = nm; }
	
	string getLastIP() { return lastClientIP; }
	string getToken() { return token; }

	uint getID() { return uid; }
	void setID(int id) { uid = id; }
	
	Server *getServer() { return server; }

	void setConnection(ConnectionPtr conn);
	ConnectionPtr getConnection() { return connection; }

	void onPacket(string pack);
	void onDisconnect();
	void onRemove();
	void onKick(RoomPtr room);
	
	MemberPtr joinRoom(RoomPtr room);
	void leaveRoom(RoomPtr room);

	RoomPtr getRoomByName(const string &name);
	inline const set<RoomPtr> &getConnectedRooms() { return rooms; }

	void sendPacket(const Packet &);
	void sendRawData(const string &data);

	inline bool isAdmin() {
		return uid == 1 || uid == 2;
	}

	inline bool isGuest() { return uid <= 0; }
};

#endif

