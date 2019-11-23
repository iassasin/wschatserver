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
#include <unordered_set>
#include <map>
#include <list>
#include <ctime>
#include <jsoncpp/json/json.h>

#include "server.hpp"
#include "client.hpp"

using std::vector;
using std::string;
using std::unordered_set;
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
		typing,
		stop_typing,
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
	time_t lastSeenTime;
public:
	Member(weak_ptr<Room> rm, ClientPtr cli){
		id = 0; client = cli;
		room = rm;
		status = Status::bad;
		girl = false;
		color = "gray";
		lastSeenTime = time(nullptr);
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
	void setStatus(Status stat){
		if (stat != status && status == Status::away) {
			lastSeenTime = time(nullptr);
		}
		status = stat;
	}

	bool isAdmin();
	bool isOwner();
	bool isModer();

	void sendPacket(const Packet &pack);

	time_t getLastSeenTime() { return lastSeenTime; }
};

class Room {
private:
	Server *server;
	string name;
	uint ownerId;
	weak_ptr<Room> self;

	unordered_set<MemberPtr> members;
	unordered_map<uint, MemberInfo> membersInfo;
	unordered_set<string> bannedNicks;
	unordered_set<string> bannedIps;
	unordered_set<uint> bannedUids;

	unordered_set<uint> moderators;

	list<string> history;

	uint nextMemberId;

	uint genNextMemberId();
	void addToHistory(const Packet &pack);
public:
	Room(Server *srv);
	~Room();

	void setSelfPtr(weak_ptr<Room> ptr){ self = ptr; }

	void setOwner(uint nid);
	inline uint getOwner(){ return ownerId; }

	string getName(){ return name; }
	void setName(string nm){ name = nm; }

	const list<string> &getHistory(){ return history; }

	void onCreate();
	void onDestroy();

	Json::Value serialize();
	void deserialize(const Json::Value &);

	inline const unordered_set<MemberPtr> &getMembers(){ return members; }
	inline const unordered_set<uint> &getModerators(){ return moderators; }

	inline const unordered_set<string> &getBannedNicks(){ return bannedNicks; }
	inline const unordered_set<string> &getBannedIps(){ return bannedIps; }
	inline const unordered_set<uint> &getBannedUids(){ return bannedUids; }

	inline bool isBannedNick(const string &nick){ return bannedNicks.find(nick) != bannedNicks.end(); }

	inline bool banNick(const string &nick){ return bannedNicks.insert(nick).second; }
	inline bool banIp(const string &ip){ return bannedIps.insert(ip).second; }
	inline bool banUid(uint uid){ return bannedUids.insert(uid).second; }

	inline bool unbanNick(const string &nick){ return bannedNicks.erase(nick) > 0; }
	inline bool unbanIp(const string &ip){ return bannedIps.erase(ip) > 0; }
	inline bool unbanUid(uint uid){ return bannedUids.erase(uid) > 0; }

	inline bool addModerator(uint uid){ return moderators.insert(uid).second; }
	inline bool removeModerator(uint uid){ return moderators.erase(uid) > 0; }
	inline bool isModerator(uint uid){ return moderators.find(uid) != moderators.end(); }

	MemberPtr addMember(ClientPtr user);
	bool removeMember(ClientPtr user);

	MemberPtr findMemberByClient(ClientPtr client);
	MemberPtr findMemberByNick(string nick);
	MemberPtr findMemberById(uint id);

	MemberInfo getStoredMemberInfo(MemberPtr member);

	bool kickMember(ClientPtr user, string reason = "");
	bool kickMember(MemberPtr member, string reason = "");

	void sendPacketToAll(const Packet &pack);
};

#endif

