//
// Created by assasin on 16.07.17.
//

#ifndef BUILD_COMMAND_MODER_HPP
#define BUILD_COMMAND_MODER_HPP

#include "command.hpp"
#include "../packets.hpp"

class CommandAddModer : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		static regex r_int("^\\d+");

		auto client = member->getClient();
		auto room = member->getRoom();
		PacketSystem syspack;
		syspack.target = room->getName();

		auto &mods = room->getModerators();
		if (!client->isAdmin() && mods.size() > 10){ //TODO: constant to config
			syspack.message = "Превышен лимит на количество модераторов";
			member->sendPacket(syspack);
		}
		else if (parser.next(r_int)){
			uint uid;
			parser.read(0, uid);

			if (uid == 0){
				syspack.message = "Гостя нельзя назначить модератором";
			} else if (room->addModerator(uid)){
				syspack.message = "Модератор добавлен";
			} else {
				syspack.message = "Пользователь уже в списке модераторов";
			}
			member->sendPacket(syspack);
		}
		else {
			syspack.message = "Укажите ID аккаунта пользователя";
			member->sendPacket(syspack);
		}
	}

	virtual std::string getName() override { return "addmoder"; }
	virtual std::string getArgumentsTemplate() override { return "<uid>"; }
	virtual std::string getDescription() override { return "Сделать пользователя с указанным ID аккаунта модератором"; }
};

class CommandDelModer : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		static regex r_int("^\\d+");

		auto room = member->getRoom();
		PacketSystem syspack;
		syspack.target = room->getName();

		if (parser.next(r_int)){
			uint uid;
			parser.read(0, uid);

			if (room->removeModerator(uid)){
				syspack.message = "Модератор убран";
			} else {
				syspack.message = "Пользователь не является модератором";
			}
			member->sendPacket(syspack);
		}
		else {
			syspack.message = "Укажите ID аккаунта пользователя";
			member->sendPacket(syspack);
		}
	}

	virtual std::string getName() override { return "delmoder"; }
	virtual std::string getArgumentsTemplate() override { return "<uid>"; }
	virtual std::string getDescription() override { return "Убрать пользователя с указанным ID аккаунта из списка модераторов"; }
};

class CommandModerList : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		auto room = member->getRoom();

		auto &mods = room->getModerators();
		string res = "Модераторы:";
		for (auto mod : mods){
			res += "\n";
			res += to_string(mod);
		}

		member->sendPacket(PacketSystem(room->getName(), res));
	}

	virtual std::string getName() override { return "moderlist"; }
	virtual std::string getArgumentsTemplate() override { return ""; }
	virtual std::string getDescription() override { return "Показать список ID модераторов комнаты"; }
};

#endif //BUILD_COMMAND_MODER_HPP
