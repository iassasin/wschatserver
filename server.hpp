#ifndef SERVER_H_
#define SERVER_H_

#include <jsoncpp/json/json.h>
#include "server_wss.hpp"

class Server;
using WSServerBase = SimpleWeb::SocketServerBase<SimpleWeb::WSS>;
using WSServer = SimpleWeb::SocketServer<SimpleWeb::WSS>;

#include <set>

#include "client.hpp"
#include "packet.hpp"
#include "memcached.hpp"
#include "db.hpp"
#include "rooms.hpp"

using namespace std;
using namespace SimpleWeb;

class Server {
public:
	using SendStream = WSServerBase::SendStream;
private:
	map<shared_ptr<WSServerBase::Connection>, ClientPtr> clients;
	WSServer server;

	set<RoomPtr> rooms;
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
	ClientPtr getClientByID(int uid);
	
	vector<string> getClients();

	RoomPtr createRoom(string name);
	bool removeRoom(string name);
	RoomPtr getRoomByName(string name);
};

#endif

