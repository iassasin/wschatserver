#ifndef ROOMS_H_
#define ROOMS_H_

#include <memory>

class Member;
class Room;

using MemberPtr = std::shared_ptr<Member>;
using RoomPtr = std::shared_ptr<Room>;

#include <vector>
#include <string>
#include <cstdint>
#include <set>
#include <list>

#include "server.hpp"
#include "client.hpp"

using std::vector;
using std::string;
using std::set;
using std::list;

class Member {
public:
	enum class Status : int { bad = 0, offline, online, away, nick_change };
private:
	friend class Room;

	ClientPtr client;
	string nick;
	Status status;
public:
	Member(ClientPtr cli){ client = cli; status = Status::bad; }

	inline ClientPtr getClient(){ return client; }

	inline string getNick(){ return nick; }
	inline void setNick(string nnick){ nick = nnick; }

	Status getStatus(){ return status; }
	void setStatus(Status stat){ status = stat; }

	inline void sendPacket(const Packet &pack);
};

class Room {
private:
	Server *server;
	uint ownerId;
	set<MemberPtr> members;
	string name;
	list<string> history;
	std::weak_ptr<Room> self;

	void addToHistory(const Packet &pack);
public:
	Room(Server *srv);
	~Room();

	void setSelfPtr(std::weak_ptr<Room> ptr){ self = ptr; }

	void setOwner(uint nid);
	inline uint getOwner(){ return ownerId; }

	string getName(){ return name; }
	void setName(string nm){ name = nm; }

	const list<string> &getHistory(){ return history; }

	void onCreate();
	void onDestroy();

	inline const set<MemberPtr> &getMembers(){ return members; }
	MemberPtr addMember(ClientPtr user);
	bool removeMember(ClientPtr user);
	MemberPtr findMemberByClient(ClientPtr client);
	MemberPtr findMemberByNick(string nick);
	bool kickMember(ClientPtr user, string reason = "");
	bool kickMember(MemberPtr member, string reason = "");

	void sendPacketToAll(const Packet &pack);
};

#endif

