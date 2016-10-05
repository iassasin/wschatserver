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
}

PacketMessage::PacketMessage(const string &targ, const string &log, const string &msg, const time_t &tm) : PacketMessage(){
	login = log;
	target = targ;
	message = msg;
	msgtime = tm;
}

PacketMessage::~PacketMessage(){

}

void PacketMessage::deserialize(const Json::Value &obj){
	message = obj["message"].asString();
	target = obj["target"].asString();
	login = obj["login"].asString();
	msgtime = obj["time"].asUInt64();
}

Json::Value PacketMessage::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
	obj["target"] = target;
	obj["time"] = (Json::UInt64) msgtime;
	obj["login"] = login;
	if (message.size() > 30000){
		obj["message"] = string(message, 0, 30000);
	} else {
		obj["message"] = message;
	}
	return obj;
}

void PacketMessage::process(Client &client){
	auto server = client.getServer();

	if (!target.empty()){
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
				room->sendPacketToAll(PacketMessage(target, member->getNick(), message, time(nullptr)));
			}
		}
	}
}

bool PacketMessage::processCommand(MemberPtr member, RoomPtr room, const string &msg){
	auto client = member->getClient();
	PacketSystem pack;
	pack.target = room->getName();

	if (startsWith(msg, "/nick ")){
		string nick = regex_split(msg, regex(" +"), 2)[1];
		cout << date("[%H:%M:%S] INFO: login = ") << nick << endl;
		if (regex_match(nick, regex("^([a-zA-Z0-9-_ ]|" REGEX_ANY_RUSSIAN "){1,24}$"))){
			if (room->findMemberByNick(nick)){
				pack.message = "Такой ник уже занят";
				client->sendPacket(pack);
			} else {
				string oldnick = member->getNick();
				member->setNick(nick);

				PacketStatus spack(room, member);
				if (!oldnick.empty()){
					spack.status = Member::Status::nick_change;
					spack.data = oldnick;
				}

				room->sendPacketToAll(spack);
			}
		} else {
			pack.message = "Ник должен содержать только латинские буквоцифры и _-, и не длинее 24 символов";
			client->sendPacket(pack);
		}
	} else if (startsWith(msg, "/kick ") && client->isAdmin()){
		string nick = regex_split(msg, regex(" +"), 2)[1];
		auto m = room->findMemberByNick(nick);
		if (m){
			room->kickMember(m);
		} else {
			pack.message = "Такой пользователь не найден";
			client->sendPacket(pack);
		}
	} else {
		return false;
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
		PacketStatus pack(room, m);
		list.append(pack.serialize());
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
	if (!ukey.empty()){
		Memcache cache;
		Database db;

		string id;
		if (cache.get(string("chat-key-") + ukey, id)){
			try {
				int uid = stoi(id.c_str());
	
				auto ps = db.prepare("SELECT login FROM users WHERE id = ?");
				ps->setInt(1, uid);
				auto rs = as_unique(ps->executeQuery());
				if (rs->next()){
					client.setID(uid);
					client.setName(rs->getString(1));
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
}

PacketStatus::PacketStatus(RoomPtr room, MemberPtr member, Member::Status stat, const string &dt)
	:PacketStatus()
{
	target = room->getName();
	name = member->getNick();
	status = stat;
	data = dt;
}

PacketStatus::PacketStatus(RoomPtr room, MemberPtr member, const string &dt)
	:PacketStatus(room, member, member->getStatus(), dt)
{

}

PacketStatus::PacketStatus(const string &tg, const string &nm, Member::Status stat, const string &nname)
	: PacketStatus()
{
	target = tg;
	name = nm;
	status = stat;
	data = nname;
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
	obj["status"] = (int) status;
	obj["name"] = name;
	obj["data"] = data;
	return obj;
}

void PacketStatus::process(Client &client){

}

//----

PacketJoin::PacketJoin(){
	type = Type::join;
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
		for (const string &s : room->getHistory()){
			client.sendRawData(s);
		}

		room->sendPacketToAll(PacketStatus(room, member));
		if (member->getNick().empty()){
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
	if (member){
		room->sendPacketToAll(PacketStatus(room, member, Member::Status::offline));
	}
}

