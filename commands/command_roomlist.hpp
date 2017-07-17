//
// Created by assasin on 16.07.17.
//

#ifndef BUILD_COMMAND_ROOMLIST_HPP
#define BUILD_COMMAND_ROOMLIST_HPP

#include "command.hpp"
#include "../packets.hpp"

class CommandRoomList : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		auto room = member->getRoom();
		auto server = member->getClient()->getServer();

		string rooms = "Комнаты:\n";
		for (auto r : server->getRooms()){
			rooms += to_string(r->getOwner()) + ": " + r->getName() + "\n";
		}

		member->sendPacket(PacketSystem(room->getName(), rooms));
	}

	virtual std::string getName() override { return "roomlist"; }
	virtual std::string getArgumentsTemplate() override { return ""; }
	virtual std::string getDescription() override { return "Список комнат"; }
};

#endif //BUILD_COMMAND_ROOMLIST_HPP
