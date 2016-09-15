#include <exception>
#include "packet.hpp"
#include "packets.hpp"
#include "proto.hpp"

Packet::Packet(){
	type = Type::bad;
}

Packet::~Packet(){
	
}

Packet *Packet::read(const string &data){
	ProtoObject obj = ProtoStream::deserialize(data);
	if (obj["type"].type == ProtoObject::Type::null){
		return nullptr;
	}
	
	Packet *pack = nullptr;
	switch ((Type) (int) obj["type"]){
		case Type::bad:					pack = new PacketBad(); break;
		case Type::system:				pack = new PacketSystem(); break;
		case Type::message:				pack = new PacketMessage(); break;
		case Type::online_list:			pack = new PacketOnlineList(); break;
		case Type::auth:				pack = new PacketAuth(); break;
		case Type::status:				pack = new PacketStatus(); break;
	}
	if (pack){
		try {
			pack->deserialize(obj);
		} catch (std::out_of_range e) {
			delete pack;
			pack = nullptr;
		}
	}
	
	return pack;
}

