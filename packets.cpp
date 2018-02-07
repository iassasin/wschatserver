#include "packets.hpp"
#include "client.hpp"
#include "algo.hpp"
#include "server.hpp"
#include "regex/regex.hpp"
#include "commands/commands.hpp"
#include "logger.hpp"
#include "db.hpp"
#include "gate.hpp"

#include <cstdlib>
#include <memory>
#include <sstream>

using namespace sql;
using namespace sinlib;
using namespace std;
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
	if (message.size() > 30*1024){
		message = string(message, 0, 30*1024);
	}
	replaceInvalidUtf8(message, ' ');
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
	obj["message"] = message;
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

CommandProcessor PacketMessage::cmd_all {
	new CommandHelp(),
	new CommandNick(),
	new CommandGender(),
    new CommandColor(),
	new CommandJoin(),
};

CommandProcessor PacketMessage::cmd_user {
	new CommandStyledMessage(PacketMessage::Style::me),
	new CommandStyledMessage(PacketMessage::Style::offtop),
	new CommandStyledMessage(PacketMessage::Style::event),
	new CommandPrivateMessage(),
	new CommandPrivateMessageById(),
};

CommandProcessor PacketMessage::cmd_moder {
	new CommandModerList(),
	new CommandBanList(),
	new CommandBanNick(),
	new CommandBanUid(),
	new CommandBanIp(),
	new CommandUnbanNick(),
	new CommandUnbanUid(),
	new CommandUnbanIp(),
	new CommandKick(),
	new CommandUserList(),
};

CommandProcessor PacketMessage::cmd_owner {
	new CommandAddModer(),
	new CommandDelModer(),
};

CommandProcessor PacketMessage::cmd_admin {
	new CommandRoomList(),
	new CommandIpCounter(),
};

bool PacketMessage::processCommand(MemberPtr member, RoomPtr room, const string &msg){
	if (msg[0] != '/'){
		return false;
	}

	bool badcmd = true;

	regex_parser parser(msg);
	static regex r_cmd("^/([^\\s]+)");
	static regex r_spaces("^\\s+");

	if (parser.next(r_cmd)){
		badcmd = false;
		string cmd;
		parser >> cmd;
		parser.next(r_spaces);

		if (member->isAdmin() && cmd_admin.process(cmd, member, parser)){}
		else if (member->isOwner() && cmd_owner.process(cmd, member, parser)){}
		else if (member->isModer() && cmd_moder.process(cmd, member, parser)){}
		else if (member->hasNick() && cmd_user.process(cmd, member, parser)){}
		else if (cmd_all.process(cmd, member, parser)){}
		else {
			badcmd = true;
		}
	}

	if (badcmd){
		member->sendPacket(PacketSystem(room->getName(), "Такая команда не существует или вы не вошли в чат"));
	}

	return true;
}

//----

PacketOnlineList::PacketOnlineList(){
	type = Type::online_list;
}

PacketOnlineList::PacketOnlineList(RoomPtr room) : PacketOnlineList(){
	target = room->getName();
	list = Json::Value(Json::arrayValue);
	auto members = room->getMembers();
	for (MemberPtr m : members){
		if (!m->getNick().empty()){
			PacketStatus pack(m);
			list.append(pack.serialize());
		}
	}
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
		client.sendPacket(PacketError(type, target, PacketError::Code::not_found, "Вы не подключены к комнате \"" + target + "\""));
		return;
	}

	client.sendPacket(PacketOnlineList(room));
}

//----

PacketAuth::PacketAuth(){
	type = Type::auth;
}

PacketAuth::~PacketAuth(){

}

void PacketAuth::deserialize(const Json::Value &obj){
	ukey = obj["ukey"].asString();
	api_key = obj["api_key"].asString();
	name = obj["login"].asString();
	password = obj["password"].asString();
}

Json::Value PacketAuth::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
	obj["user_id"] = user_id;
	obj["name"] = name;
	return obj;
}

void PacketAuth::process(Client &client){
	static vector<string> colors { "gray", "#f44", "dodgerblue", "aquamarine", "deeppink" };

	Database db;
	Gate gate;
	try {
		int uid = 0;

		auto initUser = [&](int uid, int gid, const string &name){
			client.setID(uid);
			client.setName(name);
			client.setGirl(gid == 4);
			client.setColor(colors[gid < (int) colors.size() ? gid : 2]);
		};

		if (!ukey.empty()){
			Memcache cache;
			string id;

			if (cache.get(string("chat-key-") + ukey, id)){
				uid = stoi(id);
			}
		}
		else if (!api_key.empty()){
			if (!gate.auth(client.getIP())){
				client.sendPacket(PacketError(type, PacketError::Code::access_denied, "Слишком частые попытки авторизации! Попробуйте позже."));
				return;
			}

			auto ps = db.prepare("SELECT user_id FROM api_keys WHERE `key` = ?");
			ps->setString(1, api_key);

			auto rs = ps.executeQuery();
			if (rs->next()){
				uid = rs->getInt(1);
				gate.auth(client.getIP(), true);
			}
		}

		if (uid != 0){
			auto ps = db.prepare("SELECT login, gid FROM users WHERE id = ?");
			ps->setInt(1, uid);

			auto rs = ps.executeQuery();
			if (rs->next()){
				initUser(uid, rs->getInt(2), rs->getString(1));
			}
		}
		else if (!name.empty() && !password.empty()){
			if (!gate.auth(client.getIP())){
				client.sendPacket(PacketError(type, PacketError::Code::access_denied, "Слишком частые попытки авторизации! Попробуйте позже."));
				return;
			}

			auto ps = db.prepare("SELECT id, login, gid FROM users WHERE login = ? AND pass = MD5(?)");
			ps->setString(1, name);
			ps->setString(2, password);

			auto rs = ps.executeQuery();
			if (rs->next()){
				initUser(rs->getInt(1), rs->getInt(3), rs->getString(2));
				gate.auth(client.getIP(), true);
			} else {
				client.sendPacket(PacketError(type, PacketError::Code::incorrect_loginpass, "Неверный логин/пароль!"));
				return;
			}
		}
	} catch (SQLException &e){
		Logger::error("SQLException code ", e.getErrorCode(), ", SQLState: ", e.getSQLState(), "\n", e.what());
		db.reconnect();
		client.sendPacket(PacketError(type, PacketError::Code::database_error, "Ошибка подключения к БД при авторизации!"));
		return;
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
	target = obj["target"].asString();
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
	auto room = client.getRoomByName(target);
	MemberPtr member = nullptr;
	if (room)
		member = room->findMemberByClient(client.getSelfPtr());

	if (status == Member::Status::away || status == Member::Status::back){
		auto nstat = status == Member::Status::back ? Member::Status::online : Member::Status::away;
		for (auto troom : client.getConnectedRooms()){
			auto mem = troom->findMemberByClient(client.getSelfPtr());
			if (mem && !mem->getNick().empty()){
				mem->setStatus(nstat);
				troom->sendPacketToAll(PacketStatus(mem, status));
			}
		}
	}
	else if (status == Member::Status::typing || status == Member::Status::stop_typing){
		if (member && !member->getNick().empty()){
			room->sendPacketToAll(PacketStatus(member, status));
		}
	}
}

//----

PacketJoin::PacketJoin(){
	type = Type::join;
	member_id = 0;
	auto_login = true;
	load_history = true;
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
	auto_login = obj["auto_login"].asBool();
	load_history = obj["load_history"].asBool();
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

	auto m = client.joinRoom(room);
	if (m){
		if (load_history){
			for (const string &s : room->getHistory()){
				client.sendRawData(s);
			}
		}

		string nick;

		if (auto_login){
			auto info = room->getStoredMemberInfo(m);
			if (info.user_id != 0){
				nick = info.nick;
				m->setGirl(info.girl);
				m->setColor(info.color);
			}

			if (!m->isModer()){
				if (room->isBannedNick(nick)){
					m->sendPacket(PacketSystem("", "Выбранный вами ранее ник (" + nick + ") запрещен, выберите другой ник"));
					nick.clear();
				}
			}

			auto member = room->findMemberByNick(nick);
			if (member){
				m->sendPacket(PacketSystem("", "Выбранный вами ранее ник (" + nick + ") занят, выберите другой ник"));
				nick.clear();
			}
		}

		if (nick.empty()){
			m->sendPacket(PacketSystem(room->getName(), "Перед началом общения укажите свой ник: /nick MyNick"));
		}
		else {
			m->setNick(nick);
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
		client.sendPacket(PacketError(type, target, PacketError::Code::not_found, "Вы не подключены к комнате \"" + target + "\""));
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
		client.sendPacket(PacketError(type, target, PacketError::Code::access_denied, "Гости не могут создавать комнаты"));
		return;
	}

	if (!regex_match(target, regex(R"(#[a-zA-Z\d\-_ \[\]\(\)]{3,24})"))){
		client.sendPacket(PacketError(type, target, PacketError::Code::invalid_target, "Недопустимое имя комнаты"));
		return;
	}

	auto server = client.getServer();
	auto room = server->createRoom(target);
	if (!room){
		client.sendPacket(PacketError(type, target, PacketError::Code::already_exists, "Такая комната уже существует"));
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
	if (client.isGuest()){
		client.sendPacket(PacketError(type, target, PacketError::Code::access_denied, "Гости не могут удалять комнаты"));
		return;
	}

	auto server = client.getServer();
	auto room = server->getRoomByName(target);

	if (!room){
		client.sendPacket(PacketError(type, target, PacketError::Code::not_found, "Такая комната не существует"));
		return;
	}

	if (client.isAdmin() || client.getID() == room->getOwner()){
		server->removeRoom(target);
		client.sendPacket(*this);
	} else {
		client.sendPacket(PacketError(type, target, PacketError::Code::access_denied, "Вы не можете удалить эту комнату"));
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
