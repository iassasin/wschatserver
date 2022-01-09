#include "packets.hpp"
#include "client.hpp"
#include "algo.hpp"
#include "regex/regex.hpp"
#include "commands/commands.hpp"
#include "logger.hpp"
#include "db.hpp"
#include "gate.hpp"
#include "src/exceptions.hpp"

#include <cstdlib>
#include <memory>

using namespace sql;
using namespace sinlib;
using namespace std;
using std::regex;

//----

Json::Value PacketError::serialize() const {
	auto obj = Packet::serialize();
	obj["type"] = (int) type;
	obj["source"] = (uint) source;
	obj["target"] = target;
	obj["code"] = (uint) code;
	obj["info"] = info;
	return obj;
}

void PacketError::process(Client &client) {

}

//----

PacketSystem::PacketSystem() { type = Type::system; }
PacketSystem::PacketSystem(const string &targ, const string &msg, std::optional<uint32_t> seqId)
		: target(targ), message(msg)
{
	type = Type::system;
	sequenceId = seqId;
}

PacketSystem::~PacketSystem() {}

Json::Value PacketSystem::serialize() const {
	auto obj = Packet::serialize();
	obj["type"] = (int) type;
	obj["message"] = message;
	obj["target"] = target;
	return obj;
}

void PacketSystem::process(Client &client) {}

//----

PacketMessage::PacketMessage() {
	type = Type::message;
	msgtime = 0;
	from_id = 0;
	to_id = 0;
	style = Style::message;
}

PacketMessage::PacketMessage(MemberPtr member, const string &msg, const time_t &tm) : PacketMessage() {
	target = member->getRoom()->getName();
	from_login = member->getNick();
	from_id = member->getId();
	color = member->getColor();
	to_id = 0;
	message = msg;
	msgtime = tm;
}

PacketMessage::PacketMessage(MemberPtr from, MemberPtr to, const string &msg, const time_t &tm) : PacketMessage(from, msg, tm) {
	to_id = to->getId();
}

PacketMessage::~PacketMessage() {

}

void PacketMessage::deserialize(const Json::Value &obj) {
	Packet::deserialize(obj);

	message = obj["message"].asString();
	target = obj["target"].asString();
	to_id = obj["to"].asUInt();
	msgtime = obj["time"].asUInt64();

	message = regex_replace(message, regex("\n{3,}"), "\n\n\n");
	if (message.size() > 30*1024) {
		message = string(message, 0, 30*1024);
	}
	replaceInvalidUtf8(message, ' ');
}

Json::Value PacketMessage::serialize() const {
	auto obj = Packet::serialize();
	obj["id"] = id;
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

void PacketMessage::process(Client &client) {
	if (!target.empty() && !message.empty()) {
		time_t curtime = time(nullptr);
		if (curtime - client.lastMessageTime > 1) {
			client.messageCounter = 0;
			client.lastMessageTime = curtime;
		}

		if (client.messageCounter > 3) {
			client.sendPacket(PacketSystem(target, "Вы слишком часто пишете!"));
			return;
		}

		if (regex_match(message, regex("\\s*"))) {
			client.sendPacket(PacketSystem(target, "Вы забыли написать текст сообщения :("));
			return;
		}

		++client.messageCounter;

		auto room = client.getRoomByName(target);
		if (!room) {
			client.sendPacket(PacketSystem("", string("Вы не можете писать в комнату \"") + target + "\""));
			return;
		}

		auto member = room->findMemberByClient(client.getSelfPtr());
		if (processCommand(member, room, message)) {
			return;
		}

		string nick = member->getNick();
		if (nick.empty()) {
			client.sendPacket(PacketSystem(target, "Перед началом общения укажите свой ник: /nick MyNick"));
			return;
		}

		PacketMessage msgPack(member, message);
		msgPack.id = std::to_string(room->newMessageId());
		room->sendPacketToAll(msgPack);
	}
}

CommandProcessor PacketMessage::cmd_all {
	new CommandHelp(),
	new CommandNick(),
	new CommandGender(),
	new CommandColor(),
};

CommandProcessor PacketMessage::cmd_user {
	new CommandStyledMessage(PacketMessage::Style::me),
	new CommandStyledMessage(PacketMessage::Style::offtop),
	new CommandStyledMessage(PacketMessage::Style::event),
	new CommandPrivateMessage(),
	new CommandPrivateMessageById(),
};

CommandProcessor PacketMessage::cmd_auth_user {
	new CommandMyRoomList(),
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

bool PacketMessage::processCommand(MemberPtr member, RoomPtr room, const string &msg) {
	if (msg[0] != '/') {
		return false;
	}

	bool badcmd = true;

	regex_parser parser(msg);
	static regex r_cmd("^/([^\\s]+)");
	static regex r_spaces("^\\s+");

	if (parser.next(r_cmd)) {
		badcmd = false;
		string cmd;
		parser >> cmd;
		parser.next(r_spaces);

		if (member->isAdmin() && cmd_admin.process(cmd, member, parser)) {}
		else if (member->isOwner() && cmd_owner.process(cmd, member, parser)) {}
		else if (member->isModer() && cmd_moder.process(cmd, member, parser)) {}
		else if (member->hasNick() && cmd_user.process(cmd, member, parser)) {}
		else if (member->getClient()->getID() > 0 && cmd_auth_user.process(cmd, member, parser)) {}
		else if (cmd_all.process(cmd, member, parser)) {}
		else {
			badcmd = true;
		}
	}

	if (badcmd) {
		member->sendPacket(PacketSystem(room->getName(), "Такая команда не существует или вы не вошли в чат"));
	}

	return true;
}

//----

PacketOnlineList::PacketOnlineList() {
	type = Type::online_list;
}

PacketOnlineList::PacketOnlineList(RoomPtr room, std::optional<uint32_t> seqId) : PacketOnlineList() {
	sequenceId = seqId;
	target = room->getName();
	list = Json::Value(Json::arrayValue);
	auto members = room->getMembers();
	for (MemberPtr m : members) {
		if (!m->getNick().empty()) {
			PacketStatus pack(m);
			list.append(pack.serialize());
		}
	}
}

PacketOnlineList::~PacketOnlineList() {

}

void PacketOnlineList::deserialize(const Json::Value &obj) {
	target = obj["target"].asString();
}

Json::Value PacketOnlineList::serialize() const {
	auto res = Packet::serialize();
	res["type"] = (int) type;
	res["target"] = target;
	res["list"] = list;
	return res;
}

void PacketOnlineList::process(Client &client) {
	auto room = client.getRoomByName(target);
	if (!room) {
		client.sendPacket(PacketError(type, target, PacketError::Code::not_found, "Вы не подключены к комнате \"" + target + "\""));
		return;
	}

	client.sendPacket(PacketOnlineList(room));
}

//----

PacketAuth::PacketAuth() {
	type = Type::auth;
}

PacketAuth::~PacketAuth() {

}

void PacketAuth::deserialize(const Json::Value &obj) {
	Packet::deserialize(obj);

	ukey = obj["ukey"].asString();
	api_key = obj["api_key"].asString();
	name = obj["login"].asString();
	password = obj["password"].asString();
	token = obj["token"].asString();
}

Json::Value PacketAuth::serialize() const {
	auto obj = Packet::serialize();
	obj["type"] = (int) type;
	obj["user_id"] = user_id;
	obj["name"] = name;
	obj["token"] = token;
	return obj;
}

void PacketAuth::process(Client &client) {
	auto connection = client.getConnection();
	auto server = client.getServer();
	static vector<string> colors { "gray", "#f44", "dodgerblue", "aquamarine", "deeppink" };

	try {
		Database db;
		Gate gate;
		Redis redis;

		try {
			int uid = 0;

			auto initUser = [&](int uid, int gid, const string &name) {
				client.setID(uid);
				client.setName(name);
				client.setGirl(gid == 4);
				client.setColor(colors[gid < (int) colors.size() ? gid : 2]);
			};

			if (!token.empty()) {
				auto clientPtr = client.getSelfPtr();
				if (ClientPtr targetClient = server->getClientByToken(token); targetClient && targetClient != clientPtr) {
					Logger::info(
						client.getLastIP(), " reviving client ", targetClient->getLastIP(),
						" [", targetClient->getID(), ", '", targetClient->getName(), "']"
					);

					user_id = targetClient->getID();
					name = targetClient->getName();
					token = targetClient->getToken();
					// must be first packet after revive
					client.sendPacket(*this);

					if (!server->reviveClient(clientPtr, targetClient)) {
						Logger::error("Something strange: target client can't be revived: ", client.getLastIP());
					}
					return;
				}
			}
			else if (!ukey.empty()) {
				redis.get("chat-key-" + ukey, uid);
			}
			else if (!api_key.empty()) {
				if (!gate.auth(client.getLastIP())) {
					client.sendPacket(PacketError(type, PacketError::Code::access_denied, "Слишком частые попытки авторизации! Попробуйте позже."));
					return;
				}

				auto ps = db.prepare("SELECT user_id FROM api_keys WHERE `key` = ?");
				ps->setString(1, api_key);

				auto rs = ps.executeQuery();
				if (rs->next()) {
					uid = rs->getInt(1);
					gate.auth(client.getLastIP(), true);
				}
			}
			else if (!name.empty() && !password.empty()) {
				// placeholder
			}
			// http-header authentication
			else {
				SimpleWeb::CaseInsensitiveMultimap cookies;
				if (auto cookie = connection->header.find("Cookie"); cookie != connection->header.end()) {
					cookies = SimpleWeb::HttpHeader::FieldValue::SemicolonSeparatedAttributes::parse(cookie->second);
				}

				if (auto sinidIt = cookies.find("sinid"); sinidIt != cookies.end()) {
					Json::Value session;
					if (redis.getJson("session:" + sinidIt->second, session)) {
						if (session.isMember("user_id")) {
							uid = std::stoi(session["user_id"].asString());
						}
					}
				}
			}

			if (uid != 0) {
				auto ps = db.prepare("SELECT login, gid FROM users WHERE id = ?");
				ps->setInt(1, uid);

				auto rs = ps.executeQuery();
				if (rs->next()) {
					initUser(uid, rs->getInt(2), rs->getString(1));
				}
			}
			else if (!name.empty() && !password.empty()) {
				if (!gate.auth(client.getLastIP())) {
					client.sendPacket(PacketError(type, PacketError::Code::access_denied, "Слишком частые попытки авторизации! Попробуйте позже."));
					return;
				}

				auto ps = db.prepare("SELECT id, login, gid FROM users WHERE login = ? AND pass = MD5(?)");
				ps->setString(1, name);
				ps->setString(2, password);

				auto rs = ps.executeQuery();
				if (rs->next()) {
					initUser(rs->getInt(1), rs->getInt(3), rs->getString(2));
					gate.auth(client.getLastIP(), true);
				} else {
					client.sendPacket(PacketError(type, PacketError::Code::incorrect_loginpass, "Неверный логин/пароль!"));
					return;
				}
			}
		} catch (SQLException &e) {
			Logger::error("[auth] SQLException code ", e.getErrorCode(), ", SQLState: ", e.getSQLState(), "\n", e.what());
			db.reconnect();
			client.sendPacket(PacketError(type, PacketError::Code::database_error, "Ошибка подключения к БД при авторизации!"));
			return;
		}
	} catch (const std::exception &e) {
		Logger::error("[auth] Exception: ", e.what());
	}

	user_id = client.getID();
	name = client.getName();
	token = client.getToken();
	client.sendPacket(*this);
}

//----

PacketStatus::PacketStatus() {
	type = Type::status;
	status = Member::Status::bad;
	member_id = 0;
	girl = false;
	user_id = 0;
	is_owner = false;
	is_moder = false;
	last_seen_time = 0;
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
	last_seen_time = member->getLastSeenTime();
}

PacketStatus::PacketStatus(MemberPtr member, const string &dt)
	:PacketStatus(member, member->getStatus(), dt)
{

}

PacketStatus::~PacketStatus() {

}

void PacketStatus::deserialize(const Json::Value &obj) {
	Packet::deserialize(obj);

	target = obj["target"].asString();
	status = (Member::Status) obj["status"].asInt();
}

Json::Value PacketStatus::serialize() const {
	auto obj = Packet::serialize();
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
	obj["last_seen_time"] = (Json::UInt64) last_seen_time;
	return obj;
}

void PacketStatus::process(Client &client) {
	auto room = client.getRoomByName(target);
	MemberPtr member = nullptr;
	if (room)
		member = room->findMemberByClient(client.getSelfPtr());

	if (status == Member::Status::away || status == Member::Status::back) {
		auto nstat = status == Member::Status::back ? Member::Status::online : Member::Status::away;
		for (auto troom : client.getConnectedRooms()) {
			auto mem = troom->findMemberByClient(client.getSelfPtr());
			if (mem && !mem->getNick().empty()) {
				mem->setStatus(nstat);
				troom->sendPacketToAll(PacketStatus(mem, status));
			}
		}
	}
	else if (status == Member::Status::typing || status == Member::Status::stop_typing) {
		if (member && !member->getNick().empty()) {
			room->sendPacketToAll(PacketStatus(member, status));
		}
	}
}

//----

PacketJoin::PacketJoin() {
	type = Type::join;
	member_id = 0;
	auto_login = true;
	load_history = true;
}

PacketJoin::PacketJoin(MemberPtr member, std::optional<uint32_t> seqId) : PacketJoin() {
	sequenceId = seqId;
	target = member->getRoom()->getName();
	member_id = member->getId();
	login = member->getNick();
}

PacketJoin::~PacketJoin() {

}

void PacketJoin::deserialize(const Json::Value &obj) {
	Packet::deserialize(obj);

	target = obj["target"].asString();
	auto_login = obj["auto_login"].asBool();
	load_history = obj["load_history"].asBool();
}

Json::Value PacketJoin::serialize() const {
	auto obj = Packet::serialize();
	obj["type"] = (int) type;
	obj["target"] = target;
	obj["member_id"] = member_id;
	obj["login"] = login;
	return obj;
}

void PacketJoin::process(Client &client) {
	if (client.getRoomByName(target)) {
		PacketError error(type, target, PacketError::Code::already_connected, "Вы уже подключены к комнате \"" + target + "\"");
		error.sequenceId = sequenceId;

		client.sendPacket(error);
		return;
	}

	auto server = client.getServer();
	auto room = server->getRoomByName(target);
	if (!room) {
		PacketError error(type, target, PacketError::Code::not_found, "Комнаты \"" + target + "\" не существует");
		error.sequenceId = sequenceId;

		client.sendPacket(error);
		return;
	}

	MemberPtr m;

	try {
		m = client.joinRoom(room);
	} catch (BannedByIPException &ex) {
		PacketError pack(Packet::Type::join, room->getName(), PacketError::Code::user_banned, "Вы были забанены");
		pack.sequenceId = sequenceId;
		client.sendPacket(pack);
	} catch (BannedByIDException &ex) {
		PacketError pack(Packet::Type::join, room->getName(), PacketError::Code::user_banned,
					client.getID() == 0
					? "Гости не могут войти в эту комнату. Авторизуйтесь на сайте"
					: "Вы были забанены"
			);
		pack.sequenceId = sequenceId;
		client.sendPacket(pack);
	}

	if (m) {
		client.sendPacket(PacketJoin(m, sequenceId));
		client.sendPacket(PacketOnlineList(room, sequenceId));

		if (load_history) {
			for (const string &s : room->getHistory()) {
				client.sendRawData(s);
			}
		}

		string nick;

		if (auto_login) {
			auto info = room->getStoredMemberInfo(m);
			if (info) {
				nick = info->nick;
				m->setGirl(info->girl);
				m->setColor(info->color);
			} else {
				nick = client.getName();
				m->setGirl(client.isGirl());
				m->setColor(client.getColor());
			}

			if (!m->isModer()) {
				if (room->isBannedNick(nick)) {
					m->sendPacket(PacketSystem("", "Выбранный вами ранее ник (" + nick + ") запрещен, выберите другой ник"));
					nick.clear();
				}
			}

			auto member = room->findMemberByNick(nick);
			if (member) {
				m->sendPacket(PacketSystem("", "Выбранный вами ранее ник (" + nick + ") занят, выберите другой ник"));
				nick.clear();
			}
		}

		m->sendPacket(PacketSystem(room->getName(), "Введите /help для получения списка всех доступных команд"));

		if (nick.empty()) {
			m->sendPacket(PacketSystem(room->getName(), "Перед началом общения укажите свой ник: /nick MyNick"));
		}
		else {
			m->setNick(nick);
		}
	}
}

//----

PacketLeave::PacketLeave() {
	type = Type::leave;
}

PacketLeave::PacketLeave(string targ, std::optional<uint32_t> seqId) : PacketLeave() {
	sequenceId = seqId;
	target = targ;
}

PacketLeave::~PacketLeave() {

}

void PacketLeave::deserialize(const Json::Value &obj) {
	Packet::deserialize(obj);
	target = obj["target"].asString();
}

Json::Value PacketLeave::serialize() const {
	auto obj = Packet::serialize();
	obj["type"] = (int) type;
	obj["target"] = target;
	return obj;
}

void PacketLeave::process(Client &client) {
	auto room = client.getRoomByName(target);
	if (!room) {
		PacketError pack(type, target, PacketError::Code::not_found, "Вы не подключены к комнате \"" + target + "\"");
		pack.sequenceId = sequenceId;
		client.sendPacket(pack);
		return;
	}

	client.leaveRoom(room);
	client.sendPacket(*this);
}

//----

PacketCreateRoom::PacketCreateRoom() {
	type = Type::create_room;
}

PacketCreateRoom::PacketCreateRoom(string targ) : PacketCreateRoom() {
	target = targ;
}

PacketCreateRoom::~PacketCreateRoom() {

}

void PacketCreateRoom::deserialize(const Json::Value &obj) {
	Packet::deserialize(obj);

	target = obj["target"].asString();
}

Json::Value PacketCreateRoom::serialize() const {
	auto obj = Packet::serialize();
	obj["type"] = (int) type;
	obj["target"] = target;
	return obj;
}

void PacketCreateRoom::process(Client &client) {
	if (client.isGuest()) {
		PacketError pack(type, target, PacketError::Code::access_denied, "Гости не могут создавать комнаты");
		pack.sequenceId = sequenceId;
		client.sendPacket(pack);
		return;
	}

	if (!regex_match(target, regex(R"(#[a-zA-Z\d\-_ \[\]\(\)]{3,24})"))) {
		PacketError pack(type, target, PacketError::Code::invalid_target, "Недопустимое имя комнаты");
		pack.sequenceId = sequenceId;
		client.sendPacket(pack);
		return;
	}

	auto server = client.getServer();
	auto room = server->createRoom(target);
	if (!room) {
		PacketError pack(type, target, PacketError::Code::already_exists, "Такая комната уже существует");
		pack.sequenceId = sequenceId;
		client.sendPacket(pack);
		return;
	}

	room->setOwner(client.getID());
	client.sendPacket(*this);
}

//----

PacketRemoveRoom::PacketRemoveRoom() {
	type = Type::remove_room;
}

PacketRemoveRoom::PacketRemoveRoom(string targ) : PacketRemoveRoom() {
	target = targ;
}

PacketRemoveRoom::~PacketRemoveRoom() {

}

void PacketRemoveRoom::deserialize(const Json::Value &obj) {
	Packet::deserialize(obj);

	target = obj["target"].asString();
}

Json::Value PacketRemoveRoom::serialize() const {
	auto obj = Packet::serialize();
	obj["type"] = (int) type;
	obj["target"] = target;
	return obj;
}

void PacketRemoveRoom::process(Client &client) {
	if (client.isGuest()) {
		PacketError pack(type, target, PacketError::Code::access_denied, "Гости не могут удалять комнаты");
		pack.sequenceId = sequenceId;
		client.sendPacket(pack);
		return;
	}

	auto server = client.getServer();
	auto room = server->getRoomByName(target);

	if (!room) {
		PacketError pack(type, target, PacketError::Code::not_found, "Такая комната не существует");
		pack.sequenceId = sequenceId;
		client.sendPacket(pack);
		return;
	}

	if (client.isAdmin() || client.getID() == room->getOwner()) {
		server->removeRoom(target);
		client.sendPacket(*this);
	} else {
		PacketError pack(type, target, PacketError::Code::access_denied, "Вы не можете удалить эту комнату");
		pack.sequenceId = sequenceId;
		client.sendPacket(pack);
		return;
	}
}

//----

PacketPing::PacketPing() {
	type = Type::ping;
}

PacketPing::~PacketPing() {

}

Json::Value PacketPing::serialize() const {
	auto obj = Packet::serialize();
	obj["type"] = (int) type;
	return obj;
}

void PacketPing::process(Client &client) {

}
