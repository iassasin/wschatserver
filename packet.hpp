#ifndef PACKET_H_
#define PACKET_H_

#include "proto.hpp"

#ifndef CLIENT_CLASS_DEFINED
class Client;
#endif

class Packet {
private:
	
public:
	enum class Type : int { bad=0, system, message, online_list, auth, status };
	
	Type type;
	
	Packet();
	virtual ~Packet();
	
	static Packet *read(const string &data);
	
	virtual void deserialize(const ProtoObject &) = 0;
	virtual ProtoObject serialize() const = 0;
	virtual void process(Client &) = 0;
};

#endif

