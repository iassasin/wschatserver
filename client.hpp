#ifndef CLIENT_H_
#define CLIENT_H_

#include "server_ws.hpp"
#include "packet.hpp"

using namespace std;
using namespace SimpleWeb;

#ifndef SERVER_CLASS_DEFINED
class Server;
#endif

class Client {
private:
	shared_ptr<SocketServerBase<WS>::Connection> connection;
	Server *server;
	string name;
	int uid;
public:
	Client(Server *srv, shared_ptr<SocketServerBase<WS>::Connection> conn){
		server = srv;
		connection = conn;
	}
	
	~Client(){
	
	}
	
	string getName(){ return name; }
	void setName(string nm){ name = nm; }
	
	int getID(){ return uid; }
	void setID(int id){ uid = id; }
	
	Server *getServer(){ return server; }
	shared_ptr<SocketServerBase<WS>::Connection> getConnection(){ return connection; }
	
	void onPacket(string pack);
	void onDisconnect();
	
	void sendPacket(const Packet &);
};

#define CLIENT_CLASS_DEFINED

#endif

