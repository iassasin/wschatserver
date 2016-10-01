#include "packets.hpp"
#include "client.hpp"
#include "algo.hpp"
#include "server.hpp"
#include <cstdlib>
#include <memory>
#include <sstream>

using namespace sql;

PacketSystem::PacketSystem(){ type = Type::system; }
PacketSystem::PacketSystem(const string &msg) : message(msg){ type = Type::system; }
PacketSystem::~PacketSystem(){}

void PacketSystem::deserialize(const Json::Value &obj){}

Json::Value PacketSystem::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
	obj["message"] = message;
	return obj;
}

void PacketSystem::process(Client &client){}

//----

PacketMessage::PacketMessage(){
	type = Type::message;
	msgtime = 0;
}

PacketMessage::PacketMessage(const string &log, const string &msg, const time_t &tm) : PacketMessage(){
	login = log;
	message = msg;
	msgtime = tm;
}

PacketMessage::~PacketMessage(){

}

void PacketMessage::deserialize(const Json::Value &obj){
	message = obj["message"].asString();
	login = obj["login"].asString();
	msgtime = obj["time"].asUInt64();
}

Json::Value PacketMessage::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
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
	PacketSystem pack;
	if (startsWith(message, "/nick ")){
		string nick = regex_split(message, regex(" +"), 2)[1];
		cout << date("[%H:%M:%S] INFO: login = ") << nick << endl;
		if (regex_match(nick, regex("^([a-zA-Z0-9-_ ]|" REGEX_ANY_RUSSIAN "){1,24}$"))){
			if (server->getClientByName(nick)){
				pack.message = "Такой ник уже занят";
				client.sendPacket(pack);
			} else {
				PacketStatus spack(nick, PacketStatus::Status::online);
				if (!client.getName().empty()){
					spack.status = PacketStatus::Status::nick_change;
					spack.name = client.getName();
					spack.data = nick;
				}
				client.setName(nick);
				server->sendPacketToAll(spack);
			}
		} else {
			pack.message = "Ник должен содержать только латинские буквоцифры и _-, и не длинее 24 символов";
			client.sendPacket(pack);
		}
	} else if (startsWith(message, "/kick ") && client.isAdmin()){
		string nick = regex_split(message, regex(" +"), 2)[1];
		auto cli = server->getClientByName(nick);
		if (cli){
			server->kick(cli);
		} else {
			pack.message = "Такой пользователь не найден";
			client.sendPacket(pack);
		}
	} else {
		if (client.getName().empty()){
			pack.message = "Перед началом общения укажите свой ник: /nick MyNick";
			client.sendPacket(pack);
		} else {
			server->sendPacketToAll(PacketMessage(client.getName(), message, time(nullptr)));
		}
	}
}

//----

PacketOnlineList::PacketOnlineList(){
	type = Type::online_list;
}

PacketOnlineList::PacketOnlineList(Client &client){
	clients = client.getServer()->getClients();
}

PacketOnlineList::~PacketOnlineList(){

}

void PacketOnlineList::deserialize(const Json::Value &obj){

}

Json::Value PacketOnlineList::serialize() const {
	Json::Value list;
	list["type"] = (int) type;
	int i = 0;
	for (string n : clients){
		list["list"][i++] = n;
	}
	return list;
}

void PacketOnlineList::process(Client &client){
	clients = client.getServer()->getClients();
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
	Memcache cache;
	Database db;
	auto srv = client.getServer();
	
	string id;
	if (cache.get(string("chat-key-") + ukey, id)){
		int uid = atoi(id.c_str());
		
		auto cli = srv->getClientByID(uid);
		if (cli){
			srv->kick(cli);
		}

		try {
			auto ps = db.prepare("SELECT login FROM users WHERE id = ?");
			ps->setInt(1, uid);
			auto rs = as_unique(ps->executeQuery());
			if (rs->next()){
				client.setID(uid);
				client.setName(rs->getString(1));
				srv->sendPacketToAll(PacketStatus(client.getName(), PacketStatus::Status::online));
			}
		} catch (SQLException &e){
			cout << date("[%H:%M:%S] ") << "# ERR: " << e.what() << endl;
			cout << "# ERR: SQLException code " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << endl;
			client.sendPacket(PacketSystem("Ошибка подключения к БД при авторизации!"));
		}
	}
}

//----

PacketStatus::PacketStatus(){
	type = Type::status;
	status = Status::bad;
}

PacketStatus::PacketStatus(const string &nm, Status stat, const string &nname)
	: PacketStatus()
{
	name = nm;
	status = stat;
	data = nname;
}

PacketStatus::~PacketStatus(){

}

void PacketStatus::deserialize(const Json::Value &obj){
	status = (Status) obj["status"].asInt();
	name = obj["name"].asString();
	data = obj["data"].asString();
}

Json::Value PacketStatus::serialize() const {
	Json::Value obj;
	obj["type"] = (int) type;
	obj["status"] = (int) status;
	obj["name"] = name;
	obj["data"] = data;
	return obj;
}

void PacketStatus::process(Client &client){
	cout << date("[%H:%M:%S] ") << "# WARN: server-only packet STATUS received" << endl;
}

