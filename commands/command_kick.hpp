//
// Created by assasin on 16.07.17.
//

#ifndef BUILD_COMMAND_KICK_HPP
#define BUILD_COMMAND_KICK_HPP

#include "command.hpp"
#include "../packets.hpp"

class CommandKick : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		auto room = member->getRoom();

		string nick = parser.suffix();

		auto m = room->findMemberByNick(nick);
		if (m){
			room->kickMember(m);
		} else {
			member->sendPacket(PacketSystem(room->getName(), "Такой пользователь не найден"));
		}
	}

	virtual std::string getName() override { return "kick"; }
	virtual std::string getArgumentsTemplate() override { return "<ник>"; }
	virtual std::string getDescription() override { return "Кик пользователя с указанным ником"; }
};

#endif //BUILD_COMMAND_KICK_HPP
