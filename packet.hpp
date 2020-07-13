#ifndef PACKET_H_
#define PACKET_H_

#include <string>
#include <memory>
#include <jsoncpp/json/json.h>

#include "client_fwd.hpp"

class Packet {
private:
	
public:
	enum class Type : int {
		error=0,
		system,
		message,
		online_list,
		auth,
		status,
		join,
		leave,
		create_room,
		remove_room,
		ping,
	};
	
	Type type;
	
	Packet();
	virtual ~Packet();
	
	static std::unique_ptr<Packet> read(const std::string &data);
	
	virtual void deserialize(const Json::Value &) = 0;
	virtual Json::Value serialize() const = 0;
	virtual void process(Client &) = 0;
};

#endif

