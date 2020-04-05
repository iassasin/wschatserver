#include "client.hpp"
#include "server.hpp"
#include "algo.hpp"
#include "packet.hpp"
#include "packets.hpp"
#include "logger.hpp"
#include "utils.hpp"

Client::Client(Server *srv, shared_ptr<WSServerBase::Connection> conn) {
	server = srv;
	connection = conn;
	uid = 0;
	lastMessageTime = time(nullptr);
	lastPacketTime = lastMessageTime;
	messageCounter = 0;
	_isGirl = false;
	color = "gray";
	clientIP = getRealClientIp(conn);

	if (auto cookie = conn->header.find("Cookie"); cookie != conn->header.end()) {
		cookies = SimpleWeb::HttpHeader::FieldValue::SemicolonSeparatedAttributes::parse(cookie->second);
	}
}

Client::~Client() {

}

void Client::onPacket(string msg){
	unique_ptr<Packet> pack(Packet::read(msg));
	if (pack){
		lastPacketTime = time(nullptr);
		pack->process(*this);
	} else {
		Logger::warn("Dropped invalid packet: ", msg);
	}
}

void Client::onDisconnect(){
	auto ptr = self.lock();
	for (RoomPtr room : rooms){
		auto member = room->findMemberByClient(ptr);
		room->removeMember(ptr);
	}
	rooms.clear();
}

void Client::onKick(RoomPtr room){
	rooms.erase(room);
}

void Client::sendPacket(const Packet &pack){
	server->sendPacket(connection, pack);
}

void Client::sendRawData(const string &data){
	server->sendRawData(connection, data);
}

MemberPtr Client::joinRoom(RoomPtr room){
	auto ptr = self.lock();
	auto member = room->addMember(ptr);

	if (member){
		rooms.insert(room);
		member->setStatus(Member::Status::online);
	}

	return member;
}

void Client::leaveRoom(RoomPtr room){
	if (rooms.erase(room) > 0){
		room->removeMember(self.lock());
	}
}

RoomPtr Client::getRoomByName(const string &name){
	for (RoomPtr room : rooms){
		if (room->getName() == name){
			return room;
		}
	}

	return nullptr;
}
