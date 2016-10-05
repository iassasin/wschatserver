#include "rooms.hpp"
#include <ctime>

inline void Member::sendPacket(const Packet &pack){
	client->sendPacket(pack);
}

Room::Room(Server *srv){
	server = srv;
	ownerId = -1;
}

Room::~Room(){

}

void Room::onCreate(){

}

void Room::onDestroy(){
	auto mems = members;
	auto ptr = self.lock();
	for (MemberPtr m : mems){
		m->client->leaveRoom(ptr);
	}
}

void Room::addToHistory(const Packet &pack){
	if (pack.type == Packet::Type::message){
		Json::FastWriter wr;
		history.push_back(wr.write(pack.serialize()));
		if (history.size() > 50){
			history.pop_front();
		}
	}
}

MemberPtr Room::findMemberByClient(ClientPtr client){
	for (MemberPtr m : members){
		if (m->client == client){
			return m;
		}
	}

	return nullptr;
}

MemberPtr Room::findMemberByNick(string nick){
	for (MemberPtr m : members){
		if (m->nick == nick){
			return m;
		}
	}

	return nullptr;
}

void Room::setOwner(uint nid){
	ownerId = nid;
}

MemberPtr Room::addMember(ClientPtr user){
	if (findMemberByClient(user)){
		return nullptr;
	}

	auto m = make_shared<Member>(user);
	m->setNick(user->getName());

	auto res = members.insert(m);
	if (res.second){
		return *res.first;
	}

	return nullptr;
}

bool Room::removeMember(ClientPtr user){
	auto m = findMemberByClient(user);
	return members.erase(m) > 0;
}

bool Room::kickMember(ClientPtr user, string reason){
	auto m = findMemberByClient(user);
	if (m){
		return kickMember(m, reason);
		return true;
	}

	return false;
}

bool Room::kickMember(MemberPtr member, string reason){
	return members.erase(member) > 0;
}

void Room::sendPacketToAll(const Packet &pack){
	addToHistory(pack);
	for (MemberPtr m : members){
		m->getClient()->sendPacket(pack);
	}
}

