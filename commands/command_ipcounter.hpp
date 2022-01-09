//
// Created by assasin on 16.07.17.
//

#ifndef BUILD_COMMAND_IPCOUNTER_HPP
#define BUILD_COMMAND_IPCOUNTER_HPP

#include "command.hpp"
#include "../packets.hpp"

class CommandIpCounter : public Command {
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		auto room = member->getRoom();
		auto server = member->getClient()->getServer();

		string res = "Подключения:\n";
		for (auto c : server->getClientsCounters()) {
			res += c.first
					+ " - (cli: " + to_string(c.second.clients) + ", conn: " + to_string(c.second.connections) + ")\n";
		}

		member->sendPacket(PacketSystem(room->getName(), res));
	}

	virtual std::string getName() override { return "ipcounter"; }
	virtual std::string getArgumentsTemplate() override { return ""; }
	virtual std::string getDescription() override { return "Показать счетчики подключений с ip"; }
};

#endif //BUILD_COMMAND_IPCOUNTER_HPP
