#ifndef PACKET_H_
#define PACKET_H_

#include <string>
#include <memory>
#include <optional>
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
	std::optional<uint32_t> sequenceId;
	
	Packet();
	virtual ~Packet();
	
	static std::unique_ptr<Packet> read(const std::string &data);
	
	virtual void deserialize(const Json::Value &);
	virtual Json::Value serialize() const;
	virtual void process(Client &) = 0;
};

#endif

