#ifndef SERVER_H_
#define SERVER_H_

#include "server_ws.hpp"
#include "client.hpp"
#include "packet.hpp"
#include "memcached.hpp"
#include "db.hpp"

using namespace std;
using namespace SimpleWeb;

class Server {
private:
	map<shared_ptr<SocketServerBase<WS>::Connection>, shared_ptr<Client> > clients;
	SocketServer<WS> server;
	Memcache cache;
	
	list<string> pack_history;
	
	void sendRawData(shared_ptr<SocketServerBase<WS>::Connection> conn, const string &rdata);
	void addToHistory(const string &spack);
public:
	Server(int port);
	
	void start();
	
	void kick(shared_ptr<Client> client);
	void sendPacket(shared_ptr<SocketServerBase<WS>::Connection> conn, const Packet &);
	void sendPacketToAll(const Packet &);
	
	shared_ptr<Client> getClientByName(string name);
	shared_ptr<Client> getClientByID(int uid);
	
	vector<string> getClients();
	Memcache &getMemcache(){ return cache; } //TODO: переделать как Database
};

#define SERVER_CLASS_DEFINED

#endif

