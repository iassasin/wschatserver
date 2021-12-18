#ifndef WSSERVER_CLIENTS_MANAGER_HPP
#define WSSERVER_CLIENTS_MANAGER_HPP

#include <vector>
#include <unordered_map>
#include <optional>

#include "../server_fwd.hpp"
#include "../client.hpp"
#include "../utils.hpp"

class ClientsManager {
public:
	struct Counter {
		uint connections;
		uint clients;
	};

	int tokenLength = 64;
public:
	ClientsManager(Server *srv) : server(srv) { }

	ClientPtr findClientByConnection(ConnectionPtr connection) {
		if (auto clientIt = connectionToClient.find(connection); clientIt != connectionToClient.end()) {
			return clientIt->second;
		}

		return {};
	}

	ClientPtr findClientByToken(const string &token) {
		if (auto clientIt = tokenToClient.find(token); clientIt != tokenToClient.end()) {
			return clientIt->second;
		}

		return {};
	}

	template<typename Predicate>
	ClientPtr findFirstClient(Predicate f) {
		for (ClientPtr &client : clients) {
			if (f(client))
				return {client};
		}

		return {};
	}

	void connect(ConnectionPtr connection) {
		string token;

		do {
			token = keygen.generate(tokenLength);
		} while (tokenToClient.find(token) != tokenToClient.end());

		ClientPtr cli = std::make_shared<Client>(server, token);
		cli->setSelfPtr(cli);
		cli->setConnection(connection);

		auto ip = cli->getLastIP();

		clients.push_back(cli);
		connectionToClient[connection] = cli;
		tokenToClient[token] = cli;

		auto &counter = countersFromIp[ip];
		++counter.clients;
		++counter.connections;
	}

	bool disconnect(ConnectionPtr connection) {
		if (auto client = findClientByConnection(connection); client) {
			disconnect(client);
			return true;
		}

		return false;
	}

	void disconnect(ClientPtr client) {
		auto connection = client->getConnection();
		if (connection && connectionToClient.erase(connection)) {
			auto ip = client->getLastIP();
			decrementForIp(ip, &Counter::connections);
			client->onDisconnect();
		}
	}

	void remove(ClientPtr client) {
		auto ip = client->getLastIP();
		auto connection = client->getConnection();

		if (connection && connectionToClient.erase(connection)) {
			decrementForIp(ip, &Counter::connections);
		}

		for (auto it = clients.begin(); it != clients.end(); ++it) {
			if (*it == client) {
				clients.erase(it);
				tokenToClient.erase(client->getToken());
				decrementForIp(ip, &Counter::clients);
				break;
			}
		}

		client->onRemove();
	}

	bool reviveClient(ClientPtr currentClient, ClientPtr targetClient) {
		auto currentConnection = currentClient->getConnection();
		if (targetClient->getConnection() || !currentConnection) {
			return false;
		}

		targetClient->setConnection(currentConnection);
		remove(currentClient);
		connectionToClient[currentConnection] = targetClient;
		targetClient->onRevive();

		return true;
	}

	const auto &getClients() { return clients; }

	const Counter getCounterFromIp(const string &ip) { return countersFromIp[ip]; }
	const unordered_map<string, Counter> &getCounters() { return countersFromIp; }
private:
	Server *server;
	Keygen keygen;

	vector<ClientPtr> clients;
	unordered_map<ConnectionPtr, ClientPtr> connectionToClient;
	unordered_map<string, Counter> countersFromIp;
	unordered_map<string, ClientPtr> tokenToClient;

	void decrementForIp(string ip, uint Counter::*field) {
		Counter &counter = countersFromIp[ip];

		--(counter.*field);
		if (counter.connections <= 0 && counter.clients <= 0) {
			countersFromIp.erase(ip);
		}
	}
};


#endif //WSSERVER_CLIENTS_MANAGER_HPP
