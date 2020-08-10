//
// Created by assasin on 17.07.17.
//

#ifndef BUILD_COMMAND_GENDER_HPP
#define BUILD_COMMAND_GENDER_HPP

#include "command.hpp"
#include "../packets.hpp"

class CommandGender : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		static regex r_to_space("^[^\\s]+");

		auto room = member->getRoom();

		string g;
		if (parser.next(r_to_space)) {
			parser.read(0, g);
			if (!(g[0] == 'f' || g[0] == 'm')) {
				g = "";
			}
		}

		if (g.empty()) {
			member->setGirl(!member->isGirl());
		} else {
			member->setGirl(g[0] == 'f');
		}

		if (member->hasNick()) {
			room->sendPacketToAll(PacketStatus(member, Member::Status::gender_change));
		} else {
			member->sendPacket(PacketStatus(member, Member::Status::gender_change));
		}
	}

	virtual std::string getName() override { return "gender"; }
	virtual std::string getArgumentsTemplate() override { return "[f|m]"; }
	virtual std::string getDescription() override { return "Сменить пол"; }
};

#endif //BUILD_COMMAND_GENDER_HPP
