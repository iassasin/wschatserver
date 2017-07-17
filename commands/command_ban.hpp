//
// Created by assasin on 16.07.17.
//

#ifndef BUILD_COMMAND_BAN_HPP
#define BUILD_COMMAND_BAN_HPP

#include "command.hpp"
#include "../packets.hpp"

class CommandBanList : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		auto room = member->getRoom();

		string res;
		auto &nlist = room->getBannedNicks();

		res += "Забаненные ники (" + to_string(nlist.size()) + "):\n";
		for (auto s : nlist){
			res += s;
			res += "\n";
		}

		auto &ulist = room->getBannedUids();
		res += "Забаненные аккаунты (" + to_string(ulist.size()) + "):\n";
		for (auto s : ulist){
			res += to_string(s);
			res += "\n";
		}

		auto &ilist = room->getBannedIps();
		res += "Забаненные IP (" + to_string(ilist.size()) + "):\n";
		for (auto s : ilist){
			res += s;
			res += "\n";
		}

		member->sendPacket(PacketSystem(room->getName(), res));
	}

	virtual std::string getName() override { return "banlist"; }
	virtual std::string getArgumentsTemplate() override { return ""; }
	virtual std::string getDescription() override { return "Показать общий список банов"; }
};

class CommandBanNick : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		auto room = member->getRoom();
		PacketSystem syspack;
		syspack.target = room->getName();

		const auto &list = room->getBannedNicks();
		string nick = parser.suffix();

		if (!member->isAdmin() && list.size() > 100){ //TODO: constant to config
			syspack.message = "Превышен лимит на количество запрещенных ников";
			member->sendPacket(syspack);
		}
		else if (nick.size() > 24 || nick.empty()){ //TODO: use regex from /nick
			syspack.message = "Некорректный ник";
			member->sendPacket(syspack);
		}
		else {
			if (room->banNick(nick)){
				auto m = room->findMemberByNick(nick);
				if (m){
					room->kickMember(m);
				}
				syspack.message = "Ник запрещен";
			} else {
				syspack.message = "Ник уже запрещен";
			}
			member->sendPacket(syspack);
		}
	}

	virtual std::string getName() override { return "bannick"; }
	virtual std::string getArgumentsTemplate() override { return "<ник>"; }
	virtual std::string getDescription() override { return "Запретить к использованию указанный ник"; }
};

class CommandBanUid : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		static regex r_int("^\\d+");

		auto room = member->getRoom();
		PacketSystem syspack;
		syspack.target = room->getName();

		auto &list = room->getBannedUids();
		if (!member->isAdmin() && list.size() > 100){ //TODO: constant to config
			syspack.message = "Превышен лимит на количество забаненных аккаунтов";
			member->sendPacket(syspack);
		}
		else if (parser.next(r_int)){
			uint uid;
			parser.read(0, uid);

			if (room->banUid(uid)){
				syspack.message = "Аккаунт забанен";
			} else {
				syspack.message = "Аккаунт уже в списке забаненных";
			}
			member->sendPacket(syspack);
		}
		else {
			syspack.message = "Укажите ID аккаунта пользователя";
			member->sendPacket(syspack);
		}
	}

	virtual std::string getName() override { return "banuid"; }
	virtual std::string getArgumentsTemplate() override { return "<id>"; }
	virtual std::string getDescription() override { return "Забанить аккаунт пользователя с указанным ID"; }
};

class CommandBanIp : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		static regex r_ip("^(\\d{1,3}\\.){3}\\d{1,3}");

		auto room = member->getRoom();
		PacketSystem syspack;
		syspack.target = room->getName();

		auto &list = room->getBannedIps();
		if (!member->isAdmin() && list.size() > 100){ //TODO: constant to config
			syspack.message = "Превышен лимит на количество забаненных IP";
			member->sendPacket(syspack);
		}
		else if (parser.next(r_ip)){
			string ip;
			parser.read(0, ip);

			if (room->banIp(ip)){
				syspack.message = "IP забанен";
			} else {
				syspack.message = "IP уже в списке забаненных";
			}
			member->sendPacket(syspack);
		}
		else {
			syspack.message = "Укажите корректный IP-адрес";
			member->sendPacket(syspack);
		}
	}

	virtual std::string getName() override { return "banip"; }
	virtual std::string getArgumentsTemplate() override { return "<ip>"; }
	virtual std::string getDescription() override { return "Забанить IP-адрес"; }
};

class CommandUnbanNick : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		auto room = member->getRoom();
		PacketSystem syspack;
		syspack.target = room->getName();

		string nick = parser.suffix();

		if (nick.size() > 24 || nick.empty()){ //TODO: use regex from /nick
			syspack.message = "Некорректный ник";
			member->sendPacket(syspack);
		}
		else {
			if (room->unbanNick(nick)){
				syspack.message = "Разбанен";
			} else {
				syspack.message = "Ник не был забанен";
			}
			member->sendPacket(syspack);
		}
	}

	virtual std::string getName() override { return "unbannick"; }
	virtual std::string getArgumentsTemplate() override { return "<ник>"; }
	virtual std::string getDescription() override { return "Убрать ник из списка запрещенных"; }
};

class CommandUnbanUid : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		static regex r_int("^\\d+");

		auto room = member->getRoom();
		PacketSystem syspack;
		syspack.target = room->getName();

		if (parser.next(r_int)){
			uint uid;
			parser.read(0, uid);

			if (room->unbanUid(uid)){
				syspack.message = "Аккаунт разбанен";
			} else {
				syspack.message = "Аккаунт не был забанен";
			}
			member->sendPacket(syspack);
		}
		else {
			syspack.message = "Укажите ID аккаунта пользователя";
			member->sendPacket(syspack);
		}
	}

	virtual std::string getName() override { return "unbanuid"; }
	virtual std::string getArgumentsTemplate() override { return "<uid>"; }
	virtual std::string getDescription() override { return "Разбанить аккаунт пользователя с указанным ID"; }
};

class CommandUnbanIp : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		static regex r_ip("^(\\d{1,3}\\.){3}\\d{1,3}");

		auto room = member->getRoom();
		PacketSystem syspack;
		syspack.target = room->getName();

		if (parser.next(r_ip)){
			string ip;
			parser.read(0, ip);

			if (room->unbanIp(ip)){
				syspack.message = "IP разбанен";
			} else {
				syspack.message = "IP не был забанен";
			}
			member->sendPacket(syspack);
		}
		else {
			syspack.message = "Укажите ID аккаунта пользователя";
			member->sendPacket(syspack);
		}
	}

	virtual std::string getName() override { return "unbanip"; }
	virtual std::string getArgumentsTemplate() override { return "<ip>"; }
	virtual std::string getDescription() override { return "Разбанить IP-адрес"; }
};

#endif //BUILD_COMMAND_BAN_HPP
