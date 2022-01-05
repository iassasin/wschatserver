#ifndef SERVER_H_
#define SERVER_H_

#include <unordered_set>
#include <unordered_map>

#include "server_fwd.hpp"
#include "client_fwd.hpp"
#include "packet.hpp"
#include "redis.hpp"
#include "rooms_fwd.hpp"
#include "src/clients_manager.hpp"

using namespace std;

class Server {
public:
	using OutMessage = WSServerBase::OutMessage;
	using InMessage = WSServerBase::InMessage;
private:
	static constexpr int WS_CLOSE_CODE_OK = 1000;

	int orphanTimeout = 5*60;
	int connectTimeout = 2.5*60;
	int pingTimeout = 1*60;
	int pingInterval = 30000;
	int maxConnectionsFromIp = 5;
	int maxClientsFromIp = 5;

	WSServer server;
	ClientsManager clientsManager;
	unordered_set<RoomPtr> rooms;

	void closeConnection(ConnectionPtr connection);
public:
	Server(const Config &config);
	~Server() { stop(); }
	
	void start();
	void stop();
	
	Json::Value serialize();
	void deserialize(const Json::Value &);

	void sendPacket(ConnectionPtr conn, const Packet &);
	void sendRawData(ConnectionPtr conn, const string &rdata);

	const unordered_set<RoomPtr> &getRooms() { return rooms; }

	RoomPtr createRoom(string name);
	bool removeRoom(string name);
	RoomPtr getRoomByName(string name);

	const unordered_map<string, ClientsManager::Counter> &getClientsCounters() { return clientsManager.getCounters(); }
	ClientPtr getClientByToken(const string &token) { return clientsManager.findClientByToken(token); }
	bool reviveClient(ClientPtr currentClient, ClientPtr targetClient);
};

#endif

