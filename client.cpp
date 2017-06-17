#include "client.hpp"
#include "server.hpp"
#include "algo.hpp"
#include "packet.hpp"
#include "packets.hpp"

void Client::onPacket(string msg){
//	cout << date("[%H:%M:%S] ") << "Receive: " << msg << endl;
	unique_ptr<Packet> pack(Packet::read(msg));
	if (pack){
		lastPacketTime = time(nullptr);
		pack->process(*this);
	} else {
		cout << date("[%H:%M:%S] ") << "Dropped invalid packet: " << msg << endl;
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

	rooms.insert(room);
	auto member = room->addMember(ptr);
	member->setStatus(Member::Status::online);

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
