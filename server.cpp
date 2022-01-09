#include <vector>
#include <map>
#include <time.h>
#include <regex>

#include "algo.hpp"
#include "client.hpp"
#include "server.hpp"
#include "packets.hpp"
#include "logger.hpp"
#include "utils.hpp"

Server::Server(const Config &config) : server(), clientsManager(this)
{
	server.config.port = (unsigned short) config["port"].asInt();
	server.config.thread_pool_size = 1;

	auto& chat = server.endpoint["^/chat/?$"];
	
	chat.on_message = [&](ConnectionPtr connection, shared_ptr<InMessage> message) {
		string msg = message->string();
		try {
			if (auto client = clientsManager.findClientByConnection(connection); client) {
				client->onPacket(msg);
			}
		} catch (const exception &e) {
			Logger::error("Exception: ", e.what(), "\nWhile processing message:", msg);
		} catch (...) {
			Logger::error("Unknown error while processing message:", msg);
		}
	};
	
	chat.on_open = [&, this](ConnectionPtr connection) {
		auto ip = getRealClientIp(connection);

		Logger::info("Opened connection from ", ip);

		auto counter = clientsManager.getCounterFromIp(ip);

		if (counter.connections >= maxConnectionsFromIp) {
			Logger::info("Connections limit reached for ", ip);
			connection->send_close(WS_CLOSE_CODE_OK);
			return;
		}

		if (counter.clients >= maxClientsFromIp) {
			Logger::info("Orphan clients limit reached for ", ip, ", try to kill oldest orphan");

			auto orphanCli = clientsManager.findFirstClient([&](ClientPtr c) {
				return !c->getConnection() && c->getLastIP() == ip;
			});
			if (orphanCli) {
				clientsManager.remove(orphanCli);
			} else {
				Logger::warn("Not found oldest orphan. Wtf? Did maxClientsFromIp < maxConnectionsFromIp?");
			}
		}

		clientsManager.connect(connection);
	};
	
	chat.on_close = [&](ConnectionPtr connection, int status, const string& reason) {
		auto ip = getRealClientIp(connection);
	    Logger::info("Closed connection from ", ip, " with status code ", status);

	    clientsManager.disconnect(connection);
	};
	
	chat.on_error = [&](auto connection, const boost::system::error_code& ec) {
		auto ip = getRealClientIp(connection);
		Logger::warn("Error in connection from ", ip,
				". Error: ", ec, ", error message: ", ec.message());

	    clientsManager.disconnect(connection);
	};

	server.runWithInterval(pingInterval, [&]{
		time_t cur = time(nullptr);
		vector<ClientPtr> toKick;

		for (auto &&cli : clientsManager.getClients()) {
			auto timeWasted = cur - cli->lastPacketTime;
			if (timeWasted > orphanTimeout) {
				toKick.push_back(cli);
			}
			else if (timeWasted > connectTimeout) {
				if (auto connection = cli->getConnection(); connection) {
					closeConnection(connection);
					clientsManager.disconnect(cli);
					Logger::info("Orphaned connection (no ping): ", cli->getName(), " [", cli->getLastIP(), "]");
				}
			}
			else if (timeWasted > pingTimeout) {
				cli->sendPacket(PacketPing());
			}
		}

		for (auto &&cli : toKick) {
			if (auto connection = cli->getConnection(); connection) {
				closeConnection(connection);
			}
			clientsManager.remove(cli);
			Logger::info("Kicked outdated orphaned connection: ", cli->getName(), " [", cli->getLastIP(), "]");
		}
	});
}

void Server::start() {
	Logger::info("Started wsserver at port ", server.config.port);
	server.start();
}

void Server::stop() {
	server.stop();
}

Json::Value Server::serialize() {
	Json::Value val;

	auto &sr = val["rooms"];
	for (RoomPtr r : rooms) {
		sr.append(r->serialize());
	}

	return val;
}

void Server::deserialize(const Json::Value &val) {
	rooms.clear();
	for (auto &v : val["rooms"]) {
		RoomPtr rm = make_shared<Room>(this);
		rm->setSelfPtr(rm);
		rm->deserialize(v);
		rooms.insert(rm);
	}
}

void Server::sendRawData(ConnectionPtr conn, const string &rdata) {
	conn->send(rdata);
}

void Server::sendPacket(ConnectionPtr conn, const Packet &pack) {
	Json::FastWriter wr;
	conn->send(wr.write(pack.serialize()));
}

RoomPtr Server::createRoom(string name) {
	auto rm = getRoomByName(name);
	if (rm)
		return nullptr;

	rm = make_shared<Room>(this);
	rm->setName(name);
	rm->setSelfPtr(rm);
	rooms.insert(rm);
	rm->onCreate();

	return rm;
}

bool Server::removeRoom(string name) {
	auto rm = getRoomByName(name);
	if (rm) {
		bool res = rooms.erase(rm) > 0;
		if (res) {
			rm->onDestroy();
		}
		return res;
	}

	return false;
}

RoomPtr Server::getRoomByName(string name) {
	for (RoomPtr room : rooms) {
		if (room->getName() == name) {
			return room;
		}
	}

	return nullptr;
}

void Server::closeConnection(ConnectionPtr connection) {
	connection->send_close(WS_CLOSE_CODE_OK, "", [connection](auto ec){
		if (ec) {
			connection->close();
		}
	});
}

bool Server::reviveClient(ClientPtr currentClient, ClientPtr targetClient) {
	auto currentConnection = currentClient->getConnection();
	auto targetConnection = targetClient->getConnection();
	auto result = clientsManager.reviveClient(currentClient, targetClient);

	if (targetConnection) {
		if (currentConnection != targetConnection) {
			closeConnection(targetConnection);
		} else {
			Logger::error("Somehow reviving client with identical connections! currentClient == targetClient => ", currentClient == targetClient ? "true" : "false");
		}
	}

	return result;
}
