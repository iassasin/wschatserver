#include "client.hpp"
#include "server.hpp"
#include "algo.hpp"
#include "packet.hpp"
#include "packets.hpp"
#include "logger.hpp"
#include "utils.hpp"

Client::Client(Server *srv, string tok) {
	server = srv;
	uid = 0;
	lastMessageTime = time(nullptr);
	lastPacketTime = lastMessageTime;
	messageCounter = 0;
	_isGirl = false;
	color = "gray";
	token = std::move(tok);
}

Client::~Client() {

}

void Client::setConnection(ConnectionPtr conn) {
	connection = conn;

	if (conn) {
		lastClientIP = getRealClientIp(conn);
	}
}

void Client::onPacket(string msg) {
	auto pack = Packet::read(msg);
	if (pack) {
		lastPacketTime = time(nullptr);
		pack->process(*this);
	} else {
		Logger::warn("Dropped invalid packet: ", msg);
	}
}

void Client::onRevive() {
	lastPacketTime = time(nullptr);
	auto ptr = self.lock();
	for (RoomPtr room : rooms) {
		auto member = room->findMemberByClient(ptr);

		member->setStatus(Member::Status::online);

		sendPacket(PacketJoin(member, {}));
		sendPacket(PacketOnlineList(room, {}));

		// TODO: load_history flag? Or dedicated PacketHistory
		for (const string &s : room->getHistory()) {
			sendRawData(s);
		}

		if (!member->getNick().empty()) {
			room->sendPacketToAll(PacketStatus(member, Member::Status::back));
		}
	}
}

void Client::onDisconnect() {
	setConnection(nullptr);

	auto ptr = self.lock();
	for (RoomPtr room : rooms) {
		auto member = room->findMemberByClient(ptr);
		member->setStatus(Member::Status::orphan);
		if (!member->getNick().empty()) {
			room->sendPacketToAll(PacketStatus(member, Member::Status::orphan));
		}
	}
}

void Client::onRemove() {
	auto ptr = self.lock();
	for (RoomPtr room : rooms) {
		auto member = room->findMemberByClient(ptr);
		room->removeMember(ptr);
	}

	rooms.clear();
	setConnection(nullptr);
}

void Client::onKick(RoomPtr room) {
	rooms.erase(room);
}

void Client::sendPacket(const Packet &pack) {
	// TODO: очередь сообщений, пока не вернулся?
	if (connection) {
		server->sendPacket(connection, pack);
	}
}

void Client::sendRawData(const string &data) {
	if (connection) {
		server->sendRawData(connection, data);
	}
}

MemberPtr Client::joinRoom(RoomPtr room) {
	auto ptr = self.lock();
	auto member = room->addMember(ptr);

	if (member) {
		rooms.insert(room);
		member->setStatus(Member::Status::online);
	}

	return member;
}

void Client::leaveRoom(RoomPtr room) {
	if (rooms.erase(room) > 0) {
		room->removeMember(self.lock());
	}
}

RoomPtr Client::getRoomByName(const string &name) {
	for (RoomPtr room : rooms) {
		if (room->getName() == name) {
			return room;
		}
	}

	return nullptr;
}
