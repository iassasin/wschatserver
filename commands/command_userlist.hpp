//
// Created by assasin on 16.07.17.
//

#ifndef BUILD_COMMAND_USERLIST_HPP
#define BUILD_COMMAND_USERLIST_HPP

#include "command.hpp"
#include "../packets.hpp"

class CommandUserList : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		auto room = member->getRoom();

		string users = "Пользователи:\n";

		for (auto m : room->getMembers()){
			auto mc = m->getClient();
			users += "#" + to_string(m->getId()) + " " + m->getNick() + " (uid " + to_string(mc->getID()) + ", " + mc->getIP() + ")\n";
		}

		member->sendPacket(PacketSystem(room->getName(), users));
	}

	virtual std::string getName() override { return "userlist"; }
	virtual std::string getArgumentsTemplate() override { return ""; }
	virtual std::string getDescription() override { return "Список клиентов с ID и IP"; }
};

#endif //BUILD_COMMAND_USERLIST_HPP
