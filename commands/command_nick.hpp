//
// Created by assasin on 16.07.17.
//

#ifndef BUILD_COMMAND_TEST_HPP
#define BUILD_COMMAND_TEST_HPP

#include "command.hpp"
#include "../packets.hpp"
#include "../logger.hpp"

class CommandNick : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		static regex r_login("^([a-zA-Z0-9\\-_\\. ]|" REGEX_ANY_RUSSIAN "){1,24}$");
		static regex r_startSpaces("^\\s+");
		static regex r_endSpaces("\\s+$");
		static regex r_longSpaces("\\s{2,}");

		auto room = member->getRoom();
		PacketSystem syspack;
		syspack.target = room->getName();

		string nick = parser.suffix();

		nick = regex_replace(nick, r_endSpaces, "");
		nick = regex_replace(nick, r_startSpaces, "");
		nick = regex_replace(nick, r_longSpaces, " ");

		if (nick.empty() || regex_match(nick, r_login)) { //TODO: regex to config?
			if (!nick.empty() && room->findMemberByNick(nick)) {
				syspack.message = "Такой ник уже занят";
				member->sendPacket(syspack);
			} else {
				member->setNick(nick);
			}
		} else {
			syspack.message = "Ник должен содержать только латинские буквоцифры, символы '._-', пробелы и не длинее 24 символов";
			member->sendPacket(syspack);
		}
	}

	virtual std::string getName() override { return "nick"; }
	virtual std::string getArgumentsTemplate() override { return "<новый ник>"; }
	virtual std::string getDescription() override { return "Сменить ник"; }
};

#endif //BUILD_COMMAND_TEST_HPP
