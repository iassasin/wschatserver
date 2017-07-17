//
// Created by assasin on 17.07.17.
//

#ifndef BUILD_COMMAND_HELP_HPP
#define BUILD_COMMAND_HELP_HPP

#include "command.hpp"
#include "../packets.hpp"

class CommandHelp : public Command {
private:
	string help_all;
	string help_user;
	string help_moder;
	string help_owner;
	string help_admin;

	string createHelpForCommands(CommandProcessor &cmdp){
		string res;
		for (auto cmd : cmdp.getCommands()){
			res += "/" + cmd->getName() + " " + cmd->getArgumentsTemplate() + "\n"
					+ "\t" + cmd->getDescription() + "\n";
		}

		return res;
	}

	void generateHelp(){
		help_all = createHelpForCommands(PacketMessage::cmd_all);
		help_user = createHelpForCommands(PacketMessage::cmd_user);
		help_moder = createHelpForCommands(PacketMessage::cmd_moder);
		help_owner = createHelpForCommands(PacketMessage::cmd_owner);
		help_admin = createHelpForCommands(PacketMessage::cmd_admin);
	}
public:
	virtual void process(MemberPtr member, regex_parser &parser) override {
		if (help_all.empty()){
			generateHelp();
		}

		auto room = member->getRoom();

		string message = "\nДоступные команды:\n" + help_all + help_user;

		if (member->isOwner()){
			message += "\nВладелец комнаты:\n" + help_owner;
		}

		if (member->isModer()){
			message += "\nМодератор комнаты:\n" + help_moder;
		}

		if (member->isAdmin()){
			message += "\nАдмин:\n" + help_admin;
		}

		member->sendPacket(PacketSystem(room->getName(), message));
	}

	virtual std::string getName() override { return "help"; }
	virtual std::string getArgumentsTemplate() override { return ""; }
	virtual std::string getDescription() override { return "Не имею представления, зачем здесь эта команда"; }
};

#endif //BUILD_COMMAND_HELP_HPP
