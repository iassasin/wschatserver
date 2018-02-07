//
// Copied from command_nick.hpp by maestroprog on 07.02.18.
//

#ifndef BUILD_COMMAND_JOIN_HPP
#define BUILD_COMMAND_JOIN_HPP

#include "command.hpp"
#include "../packets.hpp"
#include "../logger.hpp"

class CommandJoin : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		static regex r_login(R"(#[a-zA-Z\d\-_ \[\]\(\)]{3,24})");

		string roomName = parser.suffix();

		auto server = member->getClient()->getServer();
		auto room = server->getRoomByName(roomName);

		if (room == nullptr){
			PacketSystem syspack;
			syspack.target = room->getName();
			syspack.message = "Такой комнаты не существует";
			member->sendPacket(syspack);
		} else {
			room->addMember(member->getClient());
		}
	}

	virtual std::string getName() override { return "joint"; }
	virtual std::string getArgumentsTemplate() override { return "<комната>"; }
	virtual std::string getDescription() override { return "Войти в комнату"; }
};

#endif //BUILD_COMMAND_JOIN_HPP
