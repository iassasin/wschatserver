#include "packets.hpp"
#include "client.hpp"
#include "algo.hpp"
#include "server.hpp"
#include "regex/regex.hpp"

#include <cstdlib>
#include <memory>
#include <sstream>

using namespace sql;
using namespace sinlib;
using std::regex;

PacketSystem::PacketSystem(){ type = Type::system; }
PacketSystem::PacketSystem(const string &targ, const string &msg) :  target(targ), message(msg){ type = Type::system; }
PacketSystem::~PacketSystem(){}

void PacketSystem::deserialize(const Json::Value &obj){}

Json::Value PacketSystem::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
	obj["message"] = message;
	obj["target"] = target;
	return obj;
}

void PacketSystem::process(Client &client){}

//----

PacketMessage::PacketMessage(){
	type = Type::message;
	msgtime = 0;
	from_id = 0;
	to_id = 0;
	dostyle = false;
}

PacketMessage::PacketMessage(MemberPtr member, const string &msg, const time_t &tm) : PacketMessage(){
	target = member->getRoom()->getName();
	from_login = member->getNick();
	from_id = member->getId();
	color = member->getColor();
	to_id = 0;
	message = msg;
	msgtime = tm;
}

PacketMessage::PacketMessage(MemberPtr from, MemberPtr to, const string &msg, const time_t &tm) : PacketMessage(from, msg, tm){
	to_id = to->getId();
}

PacketMessage::~PacketMessage(){

}

void PacketMessage::deserialize(const Json::Value &obj){
	message = obj["message"].asString();
	target = obj["target"].asString();
	to_id = obj["to"].asUInt();
	msgtime = obj["time"].asUInt64();
}

Json::Value PacketMessage::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
	obj["target"] = target;
	obj["time"] = (Json::UInt64) msgtime;
	obj["color"] = color;
	obj["from_login"] = from_login;
	obj["from"] = from_id;
	obj["to"] = to_id;
	obj["dostyle"] = dostyle;
	if (message.size() > 30000){
		obj["message"] = string(message, 0, 30000);
	} else {
		obj["message"] = message;
	}
	return obj;
}

void PacketMessage::process(Client &client){
	if (!target.empty() && !message.empty()){
		time_t curtime = time(nullptr);
		if (curtime - client.lastMessageTime > 1){
			client.messageCounter = 0;
			client.lastMessageTime = curtime;
		}

		if (client.messageCounter > 3){
			client.sendPacket(PacketSystem(target, "Вы слишком часто пишете!"));
			return;
		}

		if (regex_match(message, regex("\\s*"))){
			client.sendPacket(PacketSystem(target, "Вы забыли написать текст сообщения :("));
			return;
		}

		++client.messageCounter;

		auto room = client.getRoomByName(target);

		if (!room){
			client.sendPacket(PacketSystem("", string("Вы не можете писать в комнату \"") + target + "\""));
			return;
		}

		auto member = room->findMemberByClient(client.getSelfPtr());
		if (!processCommand(member, room, message)){
			string nick = member->getNick();
			if (nick.empty()){
				client.sendPacket(PacketSystem(target, "Перед началом общения укажите свой ник: /nick MyNick"));
			} else {
				room->sendPacketToAll(PacketMessage(member, message));
			}
		}
	}
}

bool PacketMessage::processCommand(MemberPtr member, RoomPtr room, const string &msg){
	auto client = member->getClient();
	PacketSystem syspack;
	syspack.target = room->getName();

	if (msg[0] != '/'){
		return false;
	}

	bool badcmd = true;

	regex_parser parser(msg);
	regex r_cmd("^/([^\\s]+)");
	regex r_spaces("^\\s+");
	regex r_to_end("^.+$");
	regex r_to_space("^[^\\s]+");
	regex r_color("^#?([\\da-fA-F]{6}|[\\da-fA-F]{3})");

	if (parser.next(r_cmd)){
		badcmd = false;
		string cmd;
		parser >> cmd;
		parser.next(r_spaces);

		if (cmd == "help"){
			syspack.message = "Доступные команды:\n"
					"/help\tвсе понятно\n"
					"/nick <новый ник>\tсменить ник\n"
					"/gender [f|m]\tсменить пол\n"
					"/color <цвет в hex-формате>\tсменить цвет. Например: #f00 (красный), #f0f000 (оттенок розового). Допускается не писать знак #\n"
					"/me <сообщение>\tнаписать сообщение-действие от своего лица"
					"/msg <ник> <сообщение>\tнаписать личное сообщение в пределах комнаты (функция тестовая)";
			client->sendPacket(syspack);
		}
		else if (cmd == "nick"){
			string nick;
			if (parser.next(r_to_end)){
				parser.read(0, nick);
			}

			cout << date("[%H:%M:%S] INFO: login = ") << nick << " (" << client->getIP() << ")" << endl;
			if (nick.empty() || regex_match(nick, regex("^([a-zA-Z0-9\\-_ ]|" REGEX_ANY_RUSSIAN "){1,24}$"))){
				if (!nick.empty() && room->findMemberByNick(nick)){
					syspack.message = "Такой ник уже занят";
					client->sendPacket(syspack);
				} else {
					member->setNick(nick);
				}
			} else {
				syspack.message = "Ник должен содержать только латинские буквоцифры и _-, и не длинее 24 символов";
				client->sendPacket(syspack);
			}
		}
		else if (cmd == "kick" && client->isAdmin()){
			string nick;
			if (parser.next(r_to_end)){
				parser.read(0, nick);
			}

			auto m = room->findMemberByNick(nick);
			if (m){
				room->kickMember(m);
			} else {
				syspack.message = "Такой пользователь не найден";
				client->sendPacket(syspack);
			}
		}
		else if (cmd == "gender"){
			string g;
			if (parser.next(r_to_space)){
				parser.read(0, g);
				if (!(g[0] == 'f' || g[0] == 'm')){
					g = "";
				}
			}

			if (g.empty()){
				member->setGirl(!member->isGirl());
			} else {
				member->setGirl(g[0] == 'f');
			}

			if (member->hasNick()){
				room->sendPacketToAll(PacketStatus(member, Member::Status::gender_change));
			} else {
				member->sendPacket(PacketStatus(member, Member::Status::gender_change));
			}
		}
		else if (cmd == "color"){
			string clr;
			if (parser.next(r_color)){
				parser.read(0, clr);
				if (clr[0] != '#'){
					clr = string("#") + clr;
				}
			}

			if (clr.empty()){
				syspack.message = "Указан неверный цвет";
				member->sendPacket(syspack);
			} else {
				member->setColor(clr);

				if (member->hasNick()){
					room->sendPacketToAll(PacketStatus(member, Member::Status::color_change));
				} else {
					member->sendPacket(PacketStatus(member, Member::Status::color_change));
				}
			}
		}
		else {
			badcmd = true;
		}

		if (badcmd && member->hasNick()){
			badcmd = false;

			if (cmd == "msg"){
				string nick;
				if (parser.next(r_to_space)){
					parser.read(0, nick);
				}

				string smsg;
				if (parser.next(r_spaces) && parser.next(r_to_end)){
					parser.read(0, smsg);
				}

				if (regex_match(smsg, regex("\\s*"))){
					client->sendPacket(PacketSystem(target, "Вы забыли написать текст сообщения :("));
				} else {
					auto m2 = room->findMemberByNick(nick);
					if (!m2){
						syspack.message = "Указанный пользователь не найден";
						client->sendPacket(syspack);
					} else {
						PacketMessage pmsg(member, m2, smsg);
						client->sendPacket(pmsg);
						m2->sendPacket(pmsg);
					}
				}
			}
			else if (cmd == "me"){
				string smsg;
				if (parser.next(r_to_end)){
					parser.read(0, smsg);
				}

				if (regex_match(smsg, regex("\\s*"))){
					client->sendPacket(PacketSystem(target, "Вы забыли написать текст сообщения :("));
				} else {
					PacketMessage pmsg(member, smsg);
					pmsg.dostyle = true;
					room->sendPacketToAll(pmsg);
				}
			}
			else {
				badcmd = true;
			}
		}
	}

	if (badcmd){
		syspack.message = "Такая команда не существует или вы не вошли в чат";
		client->sendPacket(syspack);
	}

	return true;
}

//----

PacketOnlineList::PacketOnlineList(){
	type = Type::online_list;
}

PacketOnlineList::~PacketOnlineList(){

}

void PacketOnlineList::deserialize(const Json::Value &obj){
	target = obj["target"].asString();
}

Json::Value PacketOnlineList::serialize() const {
	Json::Value res;
	res["type"] = (int) type;
	res["target"] = target;
	res["list"] = list;
	return res;
}

void PacketOnlineList::process(Client &client){
	auto room = client.getRoomByName(target);
	if (!room){
		client.sendPacket(PacketSystem("", string("Не удалось получить список онлайна комнаты \"") + target + "\""));
		return;
	}

	list.clear();
	auto members = room->getMembers();
	for (MemberPtr m : members){
		if (!m->getNick().empty()){
			PacketStatus pack(m);
			if (client.isAdmin()){
				pack.user_id = m->getClient()->getID();
			}
			list.append(pack.serialize());
		}
	}

	client.sendPacket(*this);
}

//----

PacketAuth::PacketAuth(){
	type = Type::auth;
}

PacketAuth::~PacketAuth(){

}

void PacketAuth::deserialize(const Json::Value &obj){
	ukey = obj["ukey"].asString();
}

Json::Value PacketAuth::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
	obj["ukey"] = ukey;
	return obj;
}

void PacketAuth::process(Client &client){
	static vector<string> colors { "gray", "#f44", "dodgerblue", "aquamarine", "deeppink" };

	if (!ukey.empty()){
		Memcache cache;
		Database db;

		string id;
		if (cache.get(string("chat-key-") + ukey, id)){
			try {
				int uid = stoi(id.c_str());
	
				auto ps = db.prepare("SELECT login, gid FROM users WHERE id = ?");
				ps->setInt(1, uid);
				auto rs = as_unique(ps->executeQuery());
				if (rs->next()){
					int gid = rs->getInt(2);
					client.setID(uid);
					client.setName(rs->getString(1));
					client.setGirl(gid == 4);
					client.setColor(colors[gid < (int) colors.size() ? gid : 2]);
				}
			} catch (SQLException &e){
				cout << date("[%H:%M:%S] ") << "# ERR: " << e.what() << endl;
				cout << "# ERR: SQLException code " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << endl;
				client.sendPacket(PacketSystem("", "Ошибка подключения к БД при авторизации!"));
			}
		}
	}
}

//----

PacketStatus::PacketStatus(){
	type = Type::status;
	status = Member::Status::bad;
	member_id = 0;
	girl = false;
	user_id = 0;
}

PacketStatus::PacketStatus(MemberPtr member, Member::Status stat, const string &dt)
	:PacketStatus()
{
	auto room = member->getRoom();
	target = room->getName();
	name = member->getNick();
	member_id = member->getId();
	girl = member->isGirl();
	color = member->getColor();
	status = stat;
	data = dt;
}

PacketStatus::PacketStatus(MemberPtr member, const string &dt)
	:PacketStatus(member, member->getStatus(), dt)
{

}

PacketStatus::~PacketStatus(){

}

void PacketStatus::deserialize(const Json::Value &obj){
	target = obj["target"].asString();
	status = (Member::Status) obj["status"].asInt();
	name = obj["name"].asString();
	data = obj["data"].asString();
}

Json::Value PacketStatus::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
	obj["target"] = target;
	obj["name"] = name;
	obj["status"] = (int) status;
	obj["member_id"] = member_id;
	obj["user_id"] = user_id;
	obj["girl"] = girl;
	obj["color"] = color;
	obj["data"] = data;
	return obj;
}

void PacketStatus::process(Client &client){

}

//----

PacketJoin::PacketJoin(){
	type = Type::join;
	member_id = 0;
}

PacketJoin::~PacketJoin(){

}

void PacketJoin::deserialize(const Json::Value &obj){
	target = obj["target"].asString();
}

Json::Value PacketJoin::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
	obj["target"] = target;
	obj["member_id"] = member_id;
	obj["login"] = login;
	return obj;
}

void PacketJoin::process(Client &client){
	if (client.getRoomByName(target)){
		client.sendPacket(PacketSystem("", string("Вы уже подключены к комнате \"") + target + "\""));
		return;
	}

	auto server = client.getServer();
	auto room = server->getRoomByName(target);
	if (!room){
		client.sendPacket(PacketSystem("", string("Комнаты \"") + target + "\" не существует"));
		return;
	}

	auto member = client.joinRoom(room);
	if (member){
		login = member->getNick();
		member_id = member->getId();
		client.sendPacket(*this);

		if (login.empty()){
			client.sendPacket(PacketSystem(target, "Перед началом общения укажите свой ник: /nick MyNick"));
		}
	}
}

//----

PacketLeave::PacketLeave(){
	type = Type::leave;
}

PacketLeave::~PacketLeave(){

}

void PacketLeave::deserialize(const Json::Value &obj){
	target = obj["target"].asString();
}

Json::Value PacketLeave::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
	obj["target"] = target;
	return obj;
}

void PacketLeave::process(Client &client){
	auto room = client.getRoomByName(target);
	if (!room){
		client.sendPacket(PacketSystem("", string("Вы не подключены к комнате \"") + target + "\""));
		return;
	}

	auto member = room->findMemberByClient(client.getSelfPtr());
	client.leaveRoom(room);
}

