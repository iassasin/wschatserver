#include "client.hpp"
#include "server.hpp"
#include "algo.hpp"
#include "packet.hpp"
#include "packets.hpp"

void Client::onPacket(string msg){
//	cout << date("[%H:%M:%S] ") << "Receive: " << msg << endl;
	unique_ptr<Packet> pack(Packet::read(msg));
	if (pack){
		pack->process(*this);
	} else {
		cout << date("[%H:%M:%S] ") << "Dropped invalid packet: " << msg << endl;
	}
}

void Client::onDisconnect(){
	auto ptr = self.lock();
	for (RoomPtr room : rooms){
		room->removeMember(ptr);
	}
	rooms.clear();
}

void Client::sendPacket(const Packet &pack){
	server->sendPacket(connection, pack);
}

void Client::sendRawData(const string &data){
	server->sendRawData(connection, data);
}

MemberPtr Client::joinRoom(RoomPtr room){
	rooms.insert(room);
	auto member = room->addMember(self.lock());
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
