#ifndef WSSERVER_CLIENTS_MANAGER_HPP
#define WSSERVER_CLIENTS_MANAGER_HPP

#include <vector>
#include <unordered_map>
#include <optional>

#include "../server_fwd.hpp"
#include "../client.hpp"

class ClientsManager {
public:
	struct Counter {
		uint connections;
		uint clients;
	};
public:
	ClientsManager(Server *srv) : server(srv) { }

	ClientPtr findClientByConnection(ConnectionPtr connection) {
		if (auto clientIt = connectionToClient.find(connection); clientIt != connectionToClient.end()) {
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
		ClientPtr cli = std::make_shared<Client>(server, connection);
		cli->setSelfPtr(cli);

		auto ip = cli->getIP();

		clients.push_back(cli);
		connectionToClient[connection] = cli;

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
		if (connectionToClient.erase(client->getConnection())) {
			auto ip = client->getIP();
			decrementForIp(ip, &Counter::connections);
		}
	}

	void remove(ClientPtr client) {
		auto ip = client->getIP();

		client->onDisconnect();

		if (connectionToClient.erase(client->getConnection())) {
			decrementForIp(ip, &Counter::connections);
		}

		for (auto it = clients.begin(); it != clients.end(); ++it) {
			if (*it == client) {
				clients.erase(it);
				decrementForIp(ip, &Counter::clients);
				break;
			}
		}
	}

	const auto &getClients() { return clients; }

	const Counter getCounterFromIp(const string &ip) { return countersFromIp[ip]; }
	const unordered_map<string, Counter> &getCounters() { return countersFromIp; }
private:
	Server *server;

	vector<ClientPtr> clients;
	unordered_map<ConnectionPtr, ClientPtr> connectionToClient;
	unordered_map<string, Counter> countersFromIp;

	void decrementForIp(string ip, uint Counter::*field) {
		Counter &counter = countersFromIp[ip];

		--(counter.*field);
		if (counter.connections <= 0 && counter.clients <= 0) {
			countersFromIp.erase(ip);
		}
	}
};


#endif //WSSERVER_CLIENTS_MANAGER_HPP
