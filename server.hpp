#ifndef SERVER_H_
#define SERVER_H_

class Server;

#include <set>

#include "server_ws.hpp"
#include "client.hpp"
#include "packet.hpp"
#include "memcached.hpp"
#include "db.hpp"
#include "rooms.hpp"

using namespace std;
using namespace SimpleWeb;

class Server {
private:
	map<shared_ptr<SocketServerBase<WS>::Connection>, ClientPtr> clients;
	SocketServer<WS> server;

	set<RoomPtr> rooms;
	
public:
	Server(int port);
	
	void start();
	
	void kick(ClientPtr client);
	void sendPacket(shared_ptr<SocketServerBase<WS>::Connection> conn, const Packet &);
	void sendPacketToAll(const Packet &);
	void sendRawData(shared_ptr<SocketServerBase<WS>::Connection> conn, const string &rdata);
	
	ClientPtr getClientByName(string name);
	ClientPtr getClientByID(int uid);
	
	vector<string> getClients();

	RoomPtr createRoom(string name);
	bool removeRoom(string name);
	RoomPtr getRoomByName(string name);
};

#endif

