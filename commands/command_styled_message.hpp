//
// Created by assasin on 16.07.17.
//

#ifndef BUILD_COMMAND_STYLED_MESSAGE_HPP
#define BUILD_COMMAND_STYLED_MESSAGE_HPP

#include "command.hpp"
#include "../packets.hpp"

class CommandStyledMessage : public Command {
private:
	PacketMessage::Style style;
public:
	CommandStyledMessage(PacketMessage::Style st) : style(st){}

	virtual void process(MemberPtr member, regex_parser &parser) override {
		auto room = member->getRoom();

		string smsg = parser.suffix();

		if (regex_match(smsg, regex("\\s*"))){
			member->sendPacket(PacketSystem(room->getName(), "Вы забыли написать текст сообщения :("));
		} else {
			PacketMessage pmsg(member, smsg);
			pmsg.style = style;
			room->sendPacketToAll(pmsg);
		}
	}

	virtual std::string getName() override {
		switch (style){
			case PacketMessage::Style::me: return "me";
			case PacketMessage::Style::event: return "do";
			case PacketMessage::Style::offtop: return "n";
		}
		return "?";
	}

	virtual std::string getArgumentsTemplate() override {
		return "<сообщение>";
	}

	virtual std::string getDescription() override {
		switch (style){
			case PacketMessage::Style::me: return "Написать сообщение-действие от своего лица";
			case PacketMessage::Style::event: return "Написать сообщение от третьего лица";
			case PacketMessage::Style::offtop: return "Написать оффтоп-сообщение";
		}
		return "";
	}
};

#endif //BUILD_COMMAND_STYLED_MESSAGE_HPP
