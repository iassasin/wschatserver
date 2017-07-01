#include <exception>
#include <string>
#include "packet.hpp"
#include "packets.hpp"

Packet::Packet(){
	type = Type::error;
}

Packet::~Packet(){
	
}

Packet *Packet::read(const std::string &data){
	Json::Value obj;
	Json::Reader jreader;

	if (!jreader.parse(data, obj)){
		return nullptr;
	}
	
	Packet *pack = nullptr;
	switch ((Type) obj["type"].asInt()){
		case Type::error:				pack = new PacketError(); break;
		case Type::system:				pack = new PacketSystem(); break;
		case Type::message:				pack = new PacketMessage(); break;
		case Type::online_list:			pack = new PacketOnlineList(); break;
		case Type::auth:				pack = new PacketAuth(); break;
		case Type::status:				pack = new PacketStatus(); break;
		case Type::join:				pack = new PacketJoin(); break;
		case Type::leave:				pack = new PacketLeave(); break;
		case Type::create_room:			pack = new PacketCreateRoom(); break;
		case Type::remove_room:			pack = new PacketRemoveRoom(); break;
		case Type::ping:				pack = new PacketPing(); break;
	}

	if (pack){
		try {
			pack->deserialize(obj);
		} catch (std::out_of_range &e) {
			delete pack;
			pack = nullptr;
		}
	}
	
	return pack;
}

