#ifndef CLIENT_H_
#define CLIENT_H_

#include "client_fwd.hpp"
#include "packet.hpp"
#include "server.hpp"
#include "rooms_fwd.hpp"

using namespace std;

class Client {
private:
	shared_ptr<WSServerBase::Connection> connection;
	Server *server;
	set<RoomPtr> rooms;
	weak_ptr<Client> self;

	string name;
	uint uid;
	bool _isGirl;
	string color;
	string clientIP;
	SimpleWeb::CaseInsensitiveMultimap cookies;
public:
	time_t lastPacketTime;
	time_t lastMessageTime;
	int messageCounter;

	Client(Server *srv, shared_ptr<WSServerBase::Connection> conn);
	~Client();
	
	void setSelfPtr(weak_ptr<Client> wptr) { self = wptr; }
	ClientPtr getSelfPtr() { return self.lock(); }

	inline bool isGirl() { return _isGirl; }
	inline void setGirl(bool g) { _isGirl = g; }

	inline string getColor() { return color; }
	inline void setColor(string clr) { color = clr; }

	inline string getName() { return name; }
	inline void setName(const string &nm) { name = nm; }
	
	inline string getIP() { return clientIP; }
	inline const SimpleWeb::CaseInsensitiveMultimap &getCookies() { return cookies; }

	inline uint getID() { return uid; }
	inline void setID(int id) { uid = id; }
	
	inline Server *getServer() { return server; }
	shared_ptr<WSServerBase::Connection> getConnection() { return connection; }
	
	void onPacket(string pack);
	void onDisconnect();
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

