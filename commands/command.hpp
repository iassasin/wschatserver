//
// Created by assasin on 16.07.17.
//

#ifndef BUILD_COMMAND_HPP
#define BUILD_COMMAND_HPP

#include <unordered_map>
#include <vector>
#include <initializer_list>

#include "../rooms.hpp"
#include "../regex/regex.hpp"
#include "../packets.hpp"

class Command {
public:
	virtual ~Command(){}

	virtual void process(MemberPtr member, regex_parser &cmdstr) = 0;
	virtual std::string getName() = 0;
	virtual std::string getArgumentsTemplate() = 0;
	virtual std::string getDescription() = 0;
};

class CommandProcessor {
public:
	using CommandMap = std::unordered_map<std::string, std::unique_ptr<Command>>;
private:
	CommandMap commands;
	std::vector<Command *> ordered_commands;
public:
	CommandProcessor(std::initializer_list<Command *> l){
		for (auto cmd : l){
			if (cmd){
				commands[cmd->getName()].reset(cmd);
				ordered_commands.push_back(cmd);
			}
		}
	}

	bool process(std::string cmd, MemberPtr member, regex_parser &parser){
		auto el = commands.find(cmd);
		if (el != commands.end()){
			el->second->process(member, parser);
			return true;
		}

		return false;
	}

	const std::vector<Command *> &getCommands(){ return ordered_commands; }
};

#endif //BUILD_COMMAND_HPP
