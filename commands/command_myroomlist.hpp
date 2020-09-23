//
// Created by assasin on 23.09.20.
//

#ifndef BUILD_COMMAND_MYROOMLIST_HPP
#define BUILD_COMMAND_MYROOMLIST_HPP

#include "command.hpp"
#include "../packets.hpp"

class CommandMyRoomList : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		auto room = member->getRoom();
		auto client = member->getClient();
		auto server = client->getServer();

		string rooms = "Мои комнаты:\n";
		for (auto r : server->getRooms()) {
			if (client->getID() == r->getOwner()) {
				rooms += r->getName();
				rooms +="\n";
			}
		}

		member->sendPacket(PacketSystem(room->getName(), rooms));
	}

	virtual std::string getName() override { return "myrooms"; }
	virtual std::string getArgumentsTemplate() override { return ""; }
	virtual std::string getDescription() override { return "Список комнат, которые я создал"; }
};

#endif //BUILD_COMMAND_MYROOMLIST_HPP
