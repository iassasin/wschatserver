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
#include <map>
#include <list>
#include <jsoncpp/json/json.h>

#include "server.hpp"
#include "client.hpp"

using std::vector;
using std::string;
using std::set;
using std::list;
using std::weak_ptr;
using std::unordered_map;

struct MemberInfo {
	uint user_id;
	string nick;
	bool girl;
	string color;

	MemberInfo();
	MemberInfo(MemberPtr);

	Json::Value serialize();
	void deserialize(const Json::Value &);
};

class Member {
public:
	enum class Status : int {
		bad = 0,
		offline,
		online,
		away,
		nick_change,
		gender_change,
		color_change,
		back,
	};
private:
	friend class Room;

	uint id;
	ClientPtr client;
	weak_ptr<Room> room;
	weak_ptr<Member> self;
	string nick;
	Status status;
	bool girl;
	string color;
public:
	Member(weak_ptr<Room> rm, ClientPtr cli){
		id = 0; client = cli;
		room = rm;
		status = Status::bad;
		girl = false;
		color = "gray";
	}

	inline ClientPtr getClient(){ return client; }
	inline uint getId(){ return id; }

	inline bool hasNick(){ return !nick.empty(); }

	inline bool isGirl(){ return girl; }
	inline void setGirl(bool g){ girl = g; }

	inline string getColor(){ return color; }
	inline void setColor(string clr){ color = clr; }

	RoomPtr getRoom(){ return room.lock(); }
	MemberPtr getSelfPtr(){ return self.lock(); }
	void setSelfPtr(weak_ptr<Member> wptr){ self = wptr; }

	inline string getNick(){ return nick; }
	void setNick(const string &nnick);

	Status getStatus(){ return status; }
	void setStatus(Status stat){ status = stat; }

	bool isAdmin();
	bool isOwner();
	bool isModer();

	void sendPacket(const Packet &pack);
};

class Room {
private:
	Server *server;
	int ownerId;
	set<MemberPtr> members;
	unordered_map<uint, MemberInfo> membersInfo;
	string name;
	list<string> history;
	weak_ptr<Room> self;

	uint nextMemberId;

	uint genNextMemberId();
	void addToHistory(const Packet &pack);
public:
	Room(Server *srv);
	~Room();

	void setSelfPtr(weak_ptr<Room> ptr){ self = ptr; }

	void setOwner(int nid);
	inline int getOwner(){ return ownerId; }

	string getName(){ return name; }
	void setName(string nm){ name = nm; }

	const list<string> &getHistory(){ return history; }

	void onCreate();
	void onDestroy();

	Json::Value serialize();
	void deserialize(const Json::Value &);

	inline const set<MemberPtr> &getMembers(){ return members; }

	MemberPtr addMember(ClientPtr user);
	bool removeMember(ClientPtr user);

	MemberPtr findMemberByClient(ClientPtr client);
	MemberPtr findMemberByNick(string nick);
	MemberPtr findMemberById(uint id);

	bool kickMember(ClientPtr user, string reason = "");
	bool kickMember(MemberPtr member, string reason = "");

	void sendPacketToAll(const Packet &pack);
};

#endif

