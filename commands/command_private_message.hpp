//
// Created by assasin on 16.07.17.
//

#ifndef BUILD_COMMAND_PRIVATE_MESSAGE_HPP
#define BUILD_COMMAND_PRIVATE_MESSAGE_HPP

#include "command.hpp"
#include "../packets.hpp"

class CommandPrivateMessage : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		static regex r_spaces("^\\s+");
		static regex r_to_space("^[^\\s]+");

		auto room = member->getRoom();

		string part;

		if (parser.next(r_to_space)) {
			parser.read(0, part);
		}

		string smsg;
		if (parser.next(r_spaces)) {
			smsg = parser.suffix();
		}

		if (regex_match(smsg, regex("\\s*"))) {
			member->sendPacket(PacketSystem(room->getName(), "Вы забыли написать текст сообщения :("));
		} else {
			MemberPtr m2 = room->findMemberByNick(part);

			if (!m2) {
				member->sendPacket(PacketSystem(room->getName(), "Указанный пользователь не найден"));
			} else {
				PacketMessage pmsg(member, m2, smsg);
				member->sendPacket(pmsg);
				m2->sendPacket(pmsg);
			}
		}
	}

	virtual std::string getName() override { return "msg"; }
	virtual std::string getArgumentsTemplate() override { return "<ник> <сообщение>"; }
	virtual std::string getDescription() override { return "Написать личное сообщение в пределах комнаты (функция тестовая)"; }
};

class CommandPrivateMessageById : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		static regex r_spaces("^\\s+");
		static regex r_to_space("^[^\\s]+");
		static regex r_int("^\\d+");

		auto room = member->getRoom();

		uint mid = 0;

		if (parser.next(r_int)) {
			parser.read(0, mid);
		}
		parser.next(r_to_space);

		string smsg;
		if (parser.next(r_spaces)) {
			smsg = parser.suffix();
		}

		if (regex_match(smsg, regex("\\s*"))) {
			member->sendPacket(PacketSystem(room->getName(), "Вы забыли написать текст сообщения :("));
		} else {
			MemberPtr m2 = room->findMemberById(mid);

			if (!m2 || m2->getNick().empty()) {
				member->sendPacket(PacketSystem(room->getName(), "Указанный пользователь не найден"));
			} else {
				PacketMessage pmsg(member, m2, smsg);
				member->sendPacket(pmsg);
				m2->sendPacket(pmsg);
			}
		}
	}

	virtual std::string getName() override { return "umsg"; }
	virtual std::string getArgumentsTemplate() override { return "<member_id> <сообщение>"; }
	virtual std::string getDescription() override { return "Написать личное сообщение по внутрикомнатному id пользователя (функция тестовая)"; }
};

#endif //BUILD_COMMAND_PRIVATE_MESSAGE_HPP
