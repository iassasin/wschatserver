#include <exception>
#include <string>
#include "packet.hpp"
#include "packets.hpp"

Packet::Packet(){
	type = Type::error;
}

Packet::~Packet(){
	
}

std::unique_ptr<Packet> Packet::read(const std::string &data){
	Json::Value obj;
	Json::Reader jreader;

	if (!jreader.parse(data, obj)){
		return nullptr;
	}
	
	std::unique_ptr<Packet> pack;
	switch ((Type) obj["type"].asInt()){
		case Type::error:				pack.reset(new PacketError()); break;
		case Type::system:				pack.reset(new PacketSystem()); break;
		case Type::message:				pack.reset(new PacketMessage()); break;
		case Type::online_list:			pack.reset(new PacketOnlineList()); break;
		case Type::auth:				pack.reset(new PacketAuth()); break;
		case Type::status:				pack.reset(new PacketStatus()); break;
		case Type::join:				pack.reset(new PacketJoin()); break;
		case Type::leave:				pack.reset(new PacketLeave()); break;
		case Type::create_room:			pack.reset(new PacketCreateRoom()); break;
		case Type::remove_room:			pack.reset(new PacketRemoveRoom()); break;
		case Type::ping:				pack.reset(new PacketPing()); break;
	}

	if (pack){
		try {
			pack->deserialize(obj);
		} catch (std::out_of_range &e) {
			pack = nullptr;
		}
	}
	
	return pack;
}

void Packet::deserialize(const Json::Value &obj) {
	if (obj.isMember("sequenceId")) {
		sequenceId = obj["sequenceId"].asUInt();
	}
}

Json::Value Packet::serialize() const {
	Json::Value obj;

	if (sequenceId) {
		obj["sequenceId"] = sequenceId.value();
	}

	return obj;
}