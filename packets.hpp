#ifndef PACKETS_H_
#define PACKETS_H_

#include "packet.hpp"
#include <ctime>

class PacketBad : public Packet {
public:
	virtual void deserialize(const ProtoObject &){}
	virtual ProtoObject serialize() const { return ProtoObject(); }
	virtual void process(Client &){}
};

class PacketSystem : public Packet {
private:

public:
	string message;
	
	PacketSystem();
	PacketSystem(const string &msg);
	virtual ~PacketSystem();
	
	virtual void deserialize(const ProtoObject &);
	virtual ProtoObject serialize() const;
	virtual void process(Client &);
};

class PacketMessage : public Packet {
private:

public:
	time_t msgtime;
	string login;
	string message;

	PacketMessage();
	PacketMessage(const string &log, const string &msg, const time_t &tm = 0);
	virtual ~PacketMessage();
	
	virtual void deserialize(const ProtoObject &);
	virtual ProtoObject serialize() const;
	virtual void process(Client &);
};

class PacketOnlineList : public Packet {
private:

public:
	vector<string> clients;

	PacketOnlineList();
	PacketOnlineList(Client &client);
	virtual ~PacketOnlineList();
	
	virtual void deserialize(const ProtoObject &);
	virtual ProtoObject serialize() const;
	virtual void process(Client &);
};

class PacketAuth : public Packet {
private:

public:
	string ukey;
	
	PacketAuth();
	virtual ~PacketAuth();
	
	virtual void deserialize(const ProtoObject &);
	virtual ProtoObject serialize() const;
	virtual void process(Client &);
};

class PacketStatus : public Packet {
private:

public:
	enum class Status : int { bad = 0, online, offline, nick_change };
	
	Status status;
	string name;
	string data;
	
	PacketStatus();
	PacketStatus(const string &nm, Status stat, const string &nname = "");
	virtual ~PacketStatus();
	
	virtual void deserialize(const ProtoObject &);
	virtual ProtoObject serialize() const;
	virtual void process(Client &);
};

#endif

