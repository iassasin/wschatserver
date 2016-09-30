#include "client.hpp"
#include "server.hpp"
#include "algo.hpp"
#include "packet.hpp"
#include "packets.hpp"

void Client::onPacket(string msg){
//	cout << date("[%H:%M:%S] ") << "Receive: " << msg << endl;
	unique_ptr<Packet> pack(Packet::read(msg));
	if (pack){
		pack->process(*this);
	} else {
		cout << date("[%H:%M:%S] ") << "Dropped invalid packet: " << msg << endl;
	}
}

void Client::onDisconnect(){
	if (!name.empty()){
		server->sendPacketToAll(PacketStatus(name, PacketStatus::Status::offline));
	}
}

void Client::sendPacket(const Packet &pack){
	server->sendPacket(connection, pack);
}

