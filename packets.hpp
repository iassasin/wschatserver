#ifndef PACKETS_H_
#define PACKETS_H_

#include <ctime>
#include <vector>

#include "packet.hpp"
#include "rooms.hpp"
#include "commands/command.hpp"

using std::vector;

class PacketError : public Packet {
public:
	enum class Code : uint8_t {
		unknown = 0,
		database_error,
		already_connected,
		not_found,
		access_denied,
		invalid_target,
		already_exists,
		incorrect_loginpass,
		user_banned,
	};
public:
	Type source;
	string target;
	Code code;
	string info;

	PacketError() { type = Type::error; }
	PacketError(Type src, const string &targ, Code cod, const string &inf = "")
			: source(src), target(targ), code(cod), info(inf) { type = Type::error; }
	PacketError(Type src, Code cod, const string &inf = "") : PacketError(src, "", cod, inf) {}
	PacketError(Code cod, const string &inf = "") : PacketError(Type::error, cod, inf) {}

	Json::Value serialize() const override;
	void process(Client &) override;
};

class PacketSystem : public Packet {
private:

public:
	string target;
	string message;
	
	PacketSystem();
	PacketSystem(const string &targ, const string &msg, std::optional<uint32_t> seqId = {});
	virtual ~PacketSystem();

	Json::Value serialize() const override;
	void process(Client &) override;
};

class PacketMessage : public Packet {
public:
	enum class Style : uint8_t {
		message = 0,
		me,
		event,
		offtop,
	};

	static CommandProcessor cmd_all;
	static CommandProcessor cmd_user;
	static CommandProcessor cmd_moder;
	static CommandProcessor cmd_owner;
	static CommandProcessor cmd_admin;
private:
	bool processCommand(MemberPtr member, RoomPtr room, const string &msg);
public:
	string id;
	time_t msgtime;
	string target;
	string message;
	string color;
	string from_login;
	uint from_id;
	uint to_id;
	Style style;

	PacketMessage();
	PacketMessage(MemberPtr member, const string &msg) : PacketMessage(member, msg, time(nullptr)) {}
	PacketMessage(MemberPtr from, MemberPtr to, const string &msg) : PacketMessage(from, to, msg, time(nullptr)) {}
	PacketMessage(MemberPtr member, const string &msg, const time_t &tm);
	PacketMessage(MemberPtr from, MemberPtr to, const string &msg, const time_t &tm);
	virtual ~PacketMessage();
	
	void deserialize(const Json::Value &) override;
	Json::Value serialize() const override;
	void process(Client &) override;
};

class PacketOnlineList : public Packet {
private:

public:
	string target;
	Json::Value list;

	PacketOnlineList();
	PacketOnlineList(RoomPtr room, std::optional<uint32_t> sequenceId = {});
	virtual ~PacketOnlineList();
	
	void deserialize(const Json::Value &) override;
	Json::Value serialize() const override;
	void process(Client &) override;
};

class PacketAuth : public Packet {
private:

public:
	string ukey;
	string api_key;
	uint user_id;
	string name;
	string password;
	
	PacketAuth();
	virtual ~PacketAuth();
	
	void deserialize(const Json::Value &) override;
	Json::Value serialize() const override;
	void process(Client &) override;
};

class PacketStatus : public Packet {
private:

public:
	string target;
	Member::Status status;
	uint member_id;
	uint user_id;
	bool girl;
	string color;
	string name;
	string data;
	bool is_owner;
	bool is_moder;
	time_t last_seen_time;
	
	PacketStatus();
	PacketStatus(MemberPtr member, Member::Status stat, const string &data = "");
	PacketStatus(MemberPtr member, const string &data = "");
	virtual ~PacketStatus();
	
	void deserialize(const Json::Value &) override;
	Json::Value serialize() const override;
	void process(Client &) override;
};

class PacketJoin : public Packet {
private:

public:
	string target;
	uint member_id;
	string login;
	bool auto_login;
	bool load_history;

	PacketJoin();
	PacketJoin(MemberPtr member, std::optional<uint32_t> sequenceId = {});
	virtual ~PacketJoin();

	void deserialize(const Json::Value &) override;
	Json::Value serialize() const override;
	void process(Client &) override;
};

class PacketLeave : public Packet {
private:

public:
	string target;

	PacketLeave();
	PacketLeave(string targ, std::optional<std::uint32_t> = {});
	virtual ~PacketLeave();

	void deserialize(const Json::Value &) override;
	Json::Value serialize() const override;
	void process(Client &) override;
};

class PacketCreateRoom : public Packet {
private:

public:
	string target;

	PacketCreateRoom();
	PacketCreateRoom(string targ);
	virtual ~PacketCreateRoom();

	void deserialize(const Json::Value &) override;
	Json::Value serialize() const override;
	void process(Client &) override;
};

class PacketRemoveRoom : public Packet {
private:

public:
	string target;

	PacketRemoveRoom();
	PacketRemoveRoom(string targ);
	virtual ~PacketRemoveRoom();

	void deserialize(const Json::Value &) override;
	Json::Value serialize() const override;
	void process(Client &) override;
};

class PacketPing : public Packet {
private:

public:
	PacketPing();
	virtual ~PacketPing();

	Json::Value serialize() const override;
	void process(Client &) override;
};

#endif

