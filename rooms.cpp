#include "rooms.hpp"
#include "packets.hpp"
#include <ctime>

void Member::sendPacket(const Packet &pack){
	client->sendPacket(pack);
}

void Member::setNick(const string &nnick){
	if (nick == nnick){
		return;
	}

	string oldnick = nick;
	nick = nnick;
	auto roomp = room.lock();

	PacketStatus spack(self.lock());
	if (!oldnick.empty()){
		if (nick.empty()){
			spack.status = Member::Status::offline;
			spack.name = oldnick;
		} else {
			spack.status = Member::Status::nick_change;
			spack.data = oldnick;
		}
	}

	roomp->sendPacketToAll(spack);
}

Room::Room(Server *srv){
	server = srv;
	ownerId = -1;
	nextMemberId = 0;
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

Json::Value Room::serialize(){
	Json::Value val;
	val["owner_id"] = ownerId;
	val["name"] = name;

	auto &hist = val["history"];
	for (string p : history){
		hist.append(p);
	}

	return val;
}

void Room::deserialize(const Json::Value &val){
	ownerId = val["owner_id"].asUInt();
	name = val["name"].asString();

	history.clear();
	for (auto &v : val["history"]){
		history.push_back(v.asString());
	}
}

uint Room::genNextMemberId(){
	do {
		++nextMemberId;
	} while (nextMemberId == 0 || findMemberById(nextMemberId));
	return nextMemberId;
}

void Room::addToHistory(const Packet &pack){
	if (pack.type == Packet::Type::message && ((const PacketMessage &) pack).to_id == 0){
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

MemberPtr Room::findMemberById(uint id){
	for (MemberPtr m : members){
		if (m->id == id){
			return m;
		}
	}

	return nullptr;
}

void Room::setOwner(int nid){
	ownerId = nid;
}

MemberPtr Room::addMember(ClientPtr user){
	string nick = user->getName();

	if (!nick.empty()){
		auto member = findMemberByNick(nick);
		if (member){
			kickMember(member);
		}
	}

	auto ptr = self.lock();
	auto m = make_shared<Member>(ptr, user);
	m->setSelfPtr(m);
	m->id = genNextMemberId();
	m->nick = user->getName();
	m->girl = user->isGirl();
	m->color = user->getColor();

	auto res = members.insert(m);

	user->sendPacket(PacketJoin(m));

	for (const string &s : getHistory()){
		user->sendRawData(s);
	}

	if (!m->getNick().empty()){
		sendPacketToAll(PacketStatus(m, Member::Status::online));
	}

	if (res.second){
		return *res.first;
	}

	return nullptr;
}

bool Room::removeMember(ClientPtr user){
	auto m = findMemberByClient(user);
	if (!m->getNick().empty()){
		sendPacketToAll(PacketStatus(m, Member::Status::offline));
	}

	user->sendPacket(PacketLeave(name));

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
	auto ptr = self.lock();
	member->getClient()->onKick(ptr);
	if (!member->getNick().empty()){
		sendPacketToAll(PacketStatus(member, Member::Status::offline));
	}
	return members.erase(member) > 0;
}

void Room::sendPacketToAll(const Packet &pack){
	addToHistory(pack);
	for (MemberPtr m : members){
		m->getClient()->sendPacket(pack);
	}
}

