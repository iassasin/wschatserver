//
// Created by assasin on 17.07.17.
//

#ifndef BUILD_COMMAND_COLOR_HPP
#define BUILD_COMMAND_COLOR_HPP

#include "command.hpp"
#include "../packets.hpp"

class CommandColor : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		static regex r_color("^#?([\\da-fA-F]{6}|[\\da-fA-F]{3})");

		auto room = member->getRoom();

		string clr;
		if (parser.next(r_color)) {
			parser.read(0, clr);
			if (clr[0] != '#') {
				clr = string("#") + clr;
			}
		}

		if (clr.empty()) {
			member->sendPacket(PacketSystem(room->getName(), "Указан неверный цвет"));
		} else {
			member->setColor(clr);

			if (member->hasNick()) {
				room->sendPacketToAll(PacketStatus(member, Member::Status::color_change));
			} else {
				member->sendPacket(PacketStatus(member, Member::Status::color_change));
			}
		}
	}

	virtual std::string getName() override { return "color"; }
	virtual std::string getArgumentsTemplate() override { return "<цвет в hex-формате>"; }
	virtual std::string getDescription() override { return "Сменить цвет. Например: #f00 (красный), #f0f000 (оттенок розового). Допускается не писать знак #"; }
};

#endif //BUILD_COMMAND_COLOR_HPP
