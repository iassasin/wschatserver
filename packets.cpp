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

//----

void PacketError::deserialize(const Json::Value &obj){

}

Json::Value PacketError::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
	obj["source"] = (uint) source;
	obj["target"] = target;
	obj["code"] = (uint) code;
	obj["info"] = info;
	return obj;
}

void PacketError::process(Client &client){

}

//----

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
	style = Style::message;
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

	message = regex_replace(message, regex("\n{3,}"), "\n\n\n");
	if (message.size() > 9500){
		message = string(message, 0, 9500);
	}
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
	obj["style"] = (uint) style;
	if (message.size() > 9500){
		obj["message"] = string(message, 0, 9500);
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
//	regex r_to_end("^[.\\s\\S]+$");
	regex r_to_space("^[^\\s]+");
	regex r_color("^#?([\\da-fA-F]{6}|[\\da-fA-F]{3})");
	regex r_int("\\d+");
	regex r_ip("(\\d{1,3}\\.){3}\\d{1,3}");

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
					"/me <сообщение>\tнаписать сообщение-действие от своего лица\n"
					"/do <сообщение>\tнаписать сообщение от третьего лица\n"
					"/n <сообщение>\tнаписать оффтоп-сообщение\n"
					"/msg <ник> <сообщение>\tнаписать личное сообщение в пределах комнаты (функция тестовая)";

			if (member->isOwner()){
				syspack.message += "\n\nВладелец комнаты:\n"
						"/addmoder <uid>\tсделать пользователя с указанным ID аккаунта модератором\n"
						"/delmoder <uid>\tубрать пользователя с указанным ID аккаунта из списка модераторов";
			}

			if (member->isModer()){
				syspack.message += "\n\nМодератор комнаты:\n"
						"/kick <ник>\tкик пользователя с указанным ником\n"
						"/banlist\tпоказать общий список банов\n"
						"/bannick <ник>\tзабанить пользователя с указанным ником\n"
						"/banuid <id>\tзабанить аккаунт пользователя с указанным ID\n"
						"/banip <ip>\tзабанить IP-адрес\n"
						"/unbannick <ник>\tразбанить пользователя с указанным ником\n"
						"/unbanuid <id>\tразбанить аккаунт пользователя с указанным ID\n"
						"/unbanip <ip>\tразбанить IP-адрес\n"
						"/userlist\tсписок клиентов с ID и IP\n"
						"/moderlist\tпоказать список ID модераторов комнаты";
			}

			if (client->isAdmin()){
				syspack.message += "\n\nАдмин:\n"
						"/roomlist\tсписок комнат\n"
						"/ipcounter\tпоказать счетчики подключений с ip";
			}

			client->sendPacket(syspack);
		}
		else if (cmd == "nick"){
			string nick = parser.suffix();
			nick = regex_replace(regex_replace(nick, regex("^\\s+"), ""), regex("\\s+$"), "");

			cout << date("[%H:%M:%S] INFO: login = ") << nick << " (" << client->getIP() << ")" << endl;
			if (nick.empty() || regex_match(nick, regex("^([a-zA-Z0-9\\-_ ]|" REGEX_ANY_RUSSIAN "){1,24}$"))){ //TODO: regex to config?
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

		if (badcmd && member->isOwner()){
			badcmd = false;

			if (cmd == "addmoder"){
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
			else if (cmd == "delmoder"){
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
			else {
				badcmd = true;
			}
		}

		if (badcmd && (client->isAdmin() || member->isModer())){
			badcmd = false;

			if (cmd == "kick"){
				string nick = parser.suffix();

				auto m = room->findMemberByNick(nick);
				if (m){
					room->kickMember(m);
				} else {
					syspack.message = "Такой пользователь не найден";
					client->sendPacket(syspack);
				}
			}
			else if (cmd == "bannick"){
				const auto &list = room->getBannedNicks();
				string nick = parser.suffix();

				if (!client->isAdmin() && list.size() > 100){ //TODO: constant to config
					syspack.message = "Превышен лимит на количество забаненных ников";
					client->sendPacket(syspack);
				}
				else if (nick.size() > 24 || nick.empty()){ //TODO: use regex above
					syspack.message = "Некорректный ник";
					client->sendPacket(syspack);
				}
				else {
					if (room->banNick(nick)){
						auto m = room->findMemberByNick(nick);
						if (m){
							room->kickMember(m);
						}
						syspack.message = "Забанен";
					} else {
						syspack.message = "Ник уже в бане";
					}
					client->sendPacket(syspack);
				}
			}
			else if (cmd == "unbannick"){
				string nick = parser.suffix();

				if (nick.size() > 24 || nick.empty()){ //TODO: use regex above
					syspack.message = "Некорректный ник";
					client->sendPacket(syspack);
				}
				else {
					if (room->unbanNick(nick)){
						syspack.message = "Разбанен";
					} else {
						syspack.message = "Ник не был забанен";
					}
					client->sendPacket(syspack);
				}
			}
			else if (cmd == "banip"){
				auto &list = room->getBannedIps();
				if (!client->isAdmin() && list.size() > 100){ //TODO: constant to config
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
			else if (cmd == "unbanip"){
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
			else if (cmd == "banuid"){
				auto &list = room->getBannedUids();
				if (!client->isAdmin() && list.size() > 100){ //TODO: constant to config
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
			else if (cmd == "unbanuid"){
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
			else if (cmd == "banlist"){
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

				syspack.message = res;
				client->sendPacket(syspack);
			}
			else if (cmd == "userlist"){
				string users = "Пользователи:\n";

				for (auto m : room->getMembers()){
					auto mc = m->getClient();
					users += "#" + to_string(m->getId()) + " " + m->getNick() + " (uid " + to_string(mc->getID()) + ", " + mc->getIP() + ")\n";
				}

				syspack.message = users;
				client->sendPacket(syspack);
			}
			else if (cmd == "moderlist"){
				auto &mods = room->getModerators();
				string res = "Модераторы:";
				for (auto mod : mods){
					res += "\n";
					res += to_string(mod);
				}

				syspack.message = res;
				member->sendPacket(syspack);
			}
			else {
				badcmd = true;
			}
		}

		if (badcmd && client->isAdmin()){
			badcmd = false;

			if (cmd == "roomlist"){
				auto server = client->getServer();

				string rooms = "Комнаты:\n";

				for (auto r : server->getRooms()){
					rooms += to_string(r->getOwner()) + ": " + r->getName() + "\n";
				}

				syspack.message = rooms;
				client->sendPacket(syspack);
			}
			else if (cmd == "ipcounter"){
				auto server = client->getServer();

				string res = "Подключения:\n";
				for (auto c : server->getConnectionsCounter()){
					res += c.first + " - " + to_string(c.second) + "\n";
				}

				syspack.message = res;
				member->sendPacket(syspack);
			}
			else {
				badcmd = true;
			}
		}

		if (badcmd && member->hasNick()){
			badcmd = false;

			if (cmd == "msg"){
				string nick;
				if (parser.next(r_to_space)){
					parser.read(0, nick);
				}

				string smsg;
				if (parser.next(r_spaces)){
					smsg = parser.suffix();
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
			else if (cmd == "me" || cmd == "do" || cmd == "n"){
				string smsg = parser.suffix();

				if (regex_match(smsg, regex("\\s*"))){
					client->sendPacket(PacketSystem(target, "Вы забыли написать текст сообщения :("));
				} else {
					PacketMessage pmsg(member, smsg);
					switch (cmd[0]){
						case 'm': pmsg.style = Style::me; break;
						case 'd': pmsg.style = Style::event; break;
						case 'n': pmsg.style = Style::offtop; break;
						default: pmsg.style = Style::message; break;
					}
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
		client.sendPacket(PacketSystem("", string("Не удалось получить список онлайна комнаты \"") + target + "\"")); //TODO: коды ошибок
		return;
	}

	list = Json::Value(Json::arrayValue);
	auto members = room->getMembers();
	for (MemberPtr m : members){
		if (!m->getNick().empty()){
			PacketStatus pack(m);
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
	//obj["ukey"] = ukey;
	obj["user_id"] = user_id;
	obj["name"] = name;
	return obj;
}

void PacketAuth::process(Client &client){
	static vector<string> colors { "gray", "#f44", "dodgerblue", "aquamarine", "deeppink" };

	if (!ukey.empty()){
		Memcache cache;
		Database db;

		string id;
		if (cache.get(string("chat-key-") + ukey, id)){
			int tries = 3;
			while (tries--){
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
					break;
				} catch (SQLException &e){
					cout << date("[%H:%M:%S] ") << "# ERR: " << e.what() << endl;
					cout << "# ERR: SQLException code " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << endl;
					db.reconnect();
				}
			}

			if (tries < 0){
				client.sendPacket(PacketError(type, PacketError::Code::database_error, "Ошибка подключения к БД при авторизации!"));
			}
		}
	}

	user_id = client.getID();
	name = client.getName();
	client.sendPacket(*this);
}

//----

PacketStatus::PacketStatus(){
	type = Type::status;
	status = Member::Status::bad;
	member_id = 0;
	girl = false;
	user_id = 0;
	is_owner = false;
	is_moder = false;
}

PacketStatus::PacketStatus(MemberPtr member, Member::Status stat, const string &dt)
	:PacketStatus()
{
	auto room = member->getRoom();
	target = room->getName();
	name = member->getNick();
	member_id = member->getId();
	user_id = member->getClient()->getID();
	girl = member->isGirl();
	color = member->getColor();
	status = stat;
	data = dt;
	is_owner = is_moder = member->isOwner();
	if (!is_moder) is_moder = member->isModer();
}

PacketStatus::PacketStatus(MemberPtr member, const string &dt)
	:PacketStatus(member, member->getStatus(), dt)
{

}

PacketStatus::~PacketStatus(){

}

void PacketStatus::deserialize(const Json::Value &obj){
	status = (Member::Status) obj["status"].asInt();
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
	obj["is_owner"] = is_owner;
	obj["is_moder"] = is_moder;
	return obj;
}

void PacketStatus::process(Client &client){
	if (status == Member::Status::away || status == Member::Status::back){
		auto nstat = status == Member::Status::back ? Member::Status::online : Member::Status::away;
		for (auto room : client.getConnectedRooms()){
			auto mem = room->findMemberByClient(client.getSelfPtr());
			if (mem && !mem->getNick().empty()){
				mem->setStatus(nstat);
				room->sendPacketToAll(PacketStatus(mem, status));
			}
		}
	}
}

//----

PacketJoin::PacketJoin(){
	type = Type::join;
	member_id = 0;
}

PacketJoin::PacketJoin(MemberPtr member) : PacketJoin(){
	target = member->getRoom()->getName();
	member_id = member->getId();
	login = member->getNick();
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
		client.sendPacket(PacketError(type, target, PacketError::Code::already_connected, "Вы уже подключены к комнате \"" + target + "\""));
		return;
	}

	auto server = client.getServer();
	auto room = server->getRoomByName(target);
	if (!room){
		client.sendPacket(PacketError(type, target, PacketError::Code::not_found, "Комнаты \"" + target + "\" не существует"));
		return;
	}

	auto member = client.joinRoom(room);
	if (member){
		if (member->getNick().empty()){
			client.sendPacket(PacketSystem(target, "Перед началом общения укажите свой ник: /nick MyNick"));
		}
	}
}

//----

PacketLeave::PacketLeave(){
	type = Type::leave;
}

PacketLeave::PacketLeave(string targ) : PacketLeave(){
	target = targ;
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

//----

PacketCreateRoom::PacketCreateRoom(){
	type = Type::create_room;
}

PacketCreateRoom::PacketCreateRoom(string targ) : PacketCreateRoom(){
	target = targ;
}

PacketCreateRoom::~PacketCreateRoom(){

}

void PacketCreateRoom::deserialize(const Json::Value &obj){
	target = obj["target"].asString();
}

Json::Value PacketCreateRoom::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
	obj["target"] = target;
	return obj;
}

void PacketCreateRoom::process(Client &client){
	if (client.isGuest()){
		client.sendPacket(PacketSystem("", "Гости не могут создавать комнаты"));
		return;
	}

	if (!regex_match(target, regex(R"(#[a-zA-Z\d\-_ \[\]\(\)]{3,24})"))){
		client.sendPacket(PacketSystem("", "Недопустимое имя комнаты"));
		return;
	}

	auto server = client.getServer();
	auto room = server->createRoom(target);
	if (!room){
		client.sendPacket(PacketSystem("", "Такая комната уже существует"));
		return;
	}

	room->setOwner(client.getID());
	client.sendPacket(*this);
}

//----

PacketRemoveRoom::PacketRemoveRoom(){
	type = Type::remove_room;
}

PacketRemoveRoom::PacketRemoveRoom(string targ) : PacketRemoveRoom(){
	target = targ;
}

PacketRemoveRoom::~PacketRemoveRoom(){

}

void PacketRemoveRoom::deserialize(const Json::Value &obj){
	target = obj["target"].asString();
}

Json::Value PacketRemoveRoom::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
	obj["target"] = target;
	return obj;
}

void PacketRemoveRoom::process(Client &client){
	auto server = client.getServer();
	auto room = server->getRoomByName(target);

	if (!room){
		client.sendPacket(PacketSystem("", "Такая комната уже существует"));
		return;
	}

	if (client.isAdmin() || client.getID() == room->getOwner()){
		server->removeRoom(target);
		client.sendPacket(*this);
	} else {
		client.sendPacket(PacketSystem("", "Вы не можете удалить эту комнату"));
		return;
	}
}

//----

PacketPing::PacketPing(){
	type = Type::ping;
}

PacketPing::~PacketPing(){

}

void PacketPing::deserialize(const Json::Value &obj){

}

Json::Value PacketPing::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
	return obj;
}

void PacketPing::process(Client &client){

}
