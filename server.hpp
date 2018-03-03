#ifndef SERVER_H_
#define SERVER_H_

#include <jsoncpp/json/json.h>
#include "server_wss_ex.hpp"

class Server;
using WSServerBase = SimpleWeb::SocketServerBase<SimpleWeb::WSS>;
using WSServer = WebSocketServerEx;

#include <unordered_set>
#include <unordered_map>

#include "client.hpp"
#include "packet.hpp"
#include "redis.hpp"
#include "rooms.hpp"

using namespace std;

class Server {
public:
	using SendStream = WSServerBase::SendStream;
private:
	static const int connectTimeout = 5*60;
	static const int pingTimeout = 3*60;
	static const int pingInterval = 30000;

	unordered_map<shared_ptr<WSServerBase::Connection>, ClientPtr> clients;
	unordered_map<string, uint> connectionsCountFromIp;
	WSServer server;

	unordered_set<RoomPtr> rooms;
public:
	Server(int port);
	~Server(){ stop(); }
	
	void start();
	void stop();
	
	Json::Value serialize();
	void deserialize(const Json::Value &);

	void kick(ClientPtr client);
	void sendPacket(shared_ptr<WSServerBase::Connection> conn, const Packet &);
	void sendPacketToAll(const Packet &);
	void sendRawData(shared_ptr<WSServerBase::Connection> conn, const string &rdata);
	
	ClientPtr getClientByName(string name);
	ClientPtr getClientByID(uint uid);
	
	vector<ClientPtr> getClients();
	inline const unordered_set<RoomPtr> &getRooms(){ return rooms; }

	RoomPtr createRoom(string name);
	bool removeRoom(string name);
	RoomPtr getRoomByName(string name);

	inline const unordered_map<string, uint> &getConnectionsCounter(){ return connectionsCountFromIp; }
};

#endif

