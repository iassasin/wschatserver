#ifndef DB_H_
#define DB_H_

#include <memory>
#include <pqxx/pqxx>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>
#include <memory>
#include <string>
#include <ctime>
#include <optional>
#include <format>

#include "config.hpp"
#include "algo.hpp"
#include "logger.hpp"

using std::string;
using std::unique_ptr;

struct DbUserInfo {
	int uid;
	int gid;
	std::string login;
};

class DbException : public std::runtime_error {
public:
	explicit DbException(const std::string &msg) : std::runtime_error(msg) {}
	~DbException() override = default;
};

class DatabaseMysql;

template <typename T>
class SafeStatement {
private:
	DatabaseMysql *db;
	unique_ptr<T> pst;

	template<typename F, typename R>
	R retryWithReconnect(int count, R def, F f);
public:
	SafeStatement(SafeStatement<T> &) = delete;
	SafeStatement(const SafeStatement<T> &) = delete;

	explicit SafeStatement(DatabaseMysql &fdb, T *st) {
		pst.reset(st);
		db = &fdb;
	}

	SafeStatement(SafeStatement &&ss) {
		pst.swap(ss.pst);
		db = ss.db;
	}

	SafeStatement<T> &operator = (SafeStatement<T> &&ss) {
		pst.swap(ss.pst);
		db = ss.db;
		return *this;
	}

	bool execute();
	unique_ptr<sql::ResultSet> executeQuery();
	int executeUpdate();

	T *operator -> () {
		return pst.get();
	}

	T &operator * () {
		return *pst;
	}
};

class DatabaseMysql {
private:
	static unique_ptr<sql::Connection> conn;

	bool execute(const string &sql) {
		return statement()->execute(sql);
	}

	unique_ptr<sql::ResultSet> query(const string &sql) {
		return unique_ptr<sql::ResultSet>(statement()->executeQuery(sql));
		//TODO: обертка, чтобы сохранять statement()
	}
public:
	DatabaseMysql() {
		if (!conn || conn->isClosed()) {
			reconnect();
		}
	}
	
	void reconnect() {
		sql::mysql::MySQL_Driver driver;
		auto dbconf = config["databaseMysql"];

		if (!conn) {
			sql::ConnectOptionsMap connection_properties;

			connection_properties["hostName"] = dbconf["host"].asString();
			connection_properties["userName"] = dbconf["user"].asString();
			connection_properties["password"] = dbconf["password"].asString();
			connection_properties["schema"] = dbconf["db"].asString();
			connection_properties["OPT_CHARSET_NAME"] = dbconf["charset"].asString();
			connection_properties["OPT_RECONNECT"] = true;

			conn.reset(driver.connect(connection_properties));
		} else {
			conn->reconnect();
		}
	}
	
	SafeStatement<sql::Statement> statement() {
		try {
			return SafeStatement<sql::Statement>(*this, conn->createStatement());
		} catch (sql::SQLException &e) {
			throw DbException("Mysql SQLException code " + std::to_string(e.getErrorCode()) + ", SQLState: " + e.getSQLState() + "\n" + e.what());
		}
	}
	
	SafeStatement<sql::PreparedStatement> prepare(const string &sql) {
		try {
			return SafeStatement<sql::PreparedStatement>(*this, conn->prepareStatement(sql));
		} catch (sql::SQLException &e) {
			throw DbException("Mysql SQLException code " + std::to_string(e.getErrorCode()) + ", SQLState: " + e.getSQLState() + "\n" + e.what());
		}
	}

	std::optional<int> getUserIdByApiKey(const std::string &apiKey) {
		auto ps = prepare("SELECT user_id FROM api_keys WHERE `key` = ?");
		ps->setString(1, apiKey);

		auto rs = ps.executeQuery();
		if (rs->next()) {
			return rs->getInt(1);
		}

		return {};
	}

	std::optional<DbUserInfo> getUserByUid(int uid) {
		auto ps = prepare("SELECT login, gid FROM users WHERE id = ?");
		ps->setInt(1, uid);

		auto rs = ps.executeQuery();
		if (rs->next()) {
			return {{uid, rs->getInt(2), rs->getString(1)}};
		}

		return {};
	}

	std::optional<DbUserInfo> getUserByLoginAndPassword(const std::string &login, const std::string &password) {
		auto ps = prepare("SELECT id, login, gid FROM users WHERE login = ? AND pass = MD5(?)");
		ps->setString(1, login);
		ps->setString(2, password);

		auto rs = ps.executeQuery();
		if (rs->next()) {
			return {{rs->getInt(1), rs->getInt(3), rs->getString(2)}};
		}

		return {};
	}
};

template<typename T>
template<typename F, typename R>
R SafeStatement<T>::retryWithReconnect(int count, R def, F f) {
	if (pst) {
		int tries = 0;
		while (true) {
			try {
				++tries;
				return f();
			} catch (sql::SQLException &e) {
				if (tries >= count) {
					throw;
				}

				Logger::error("Mysql SQLException code ", e.getErrorCode(), ", SQLState: ", e.getSQLState(), "\n", e.what());
				db->reconnect();
			}
		}
	}

	return def;
}

template <typename T>
bool SafeStatement<T>::execute() {
	return retryWithReconnect(3, false, [this] {
		return pst->execute();
	});
}

template <typename T>
unique_ptr<sql::ResultSet> SafeStatement<T>::executeQuery() {
	return retryWithReconnect(3, unique_ptr<sql::ResultSet>(), [this] {
		return as_unique(pst->executeQuery());
	});
}

template <typename T>
int SafeStatement<T>::executeUpdate() {
	return retryWithReconnect(3, 0, [this] {
		return pst->executeUpdate();
	});
}

//

class DatabasePostgres {
private:
	static std::unique_ptr<pqxx::connection> conn;

	pqxx::result simpleQuery(const std::string &query, const pqxx::params &params) {
		try {
			pqxx::work tx{*conn};

			auto res = tx.exec_params(query, params);
			tx.commit();

			return res;
		} catch (pqxx::sql_error &e) {
			throw DbException(std::format("Postgres SQLException in query '{}', state: {}. Message: {}", e.query(), e.sqlstate(), e.what()));
		} catch (pqxx::failure &e) {
			throw DbException(std::format("Postgres SQLException: {}", e.what()));
		}
	}
public:
	DatabasePostgres() {
		if (!conn || !conn->is_open()) {
			reconnect();
		}
	}

	void reconnect() {
		auto dbconf = config["databasePostgres"];
		auto connectionStr = std::format(
			"user={} password={} host={} port={} dbname={} options='-c search_path={}' client_encoding=UTF8",
			dbconf["user"].asString(), dbconf["password"].asString(),
			dbconf["host"].asString(), dbconf["port"].asString(),
			dbconf["db"].asString(), dbconf["db"].asString()
		);

		conn = std::make_unique<pqxx::connection>(connectionStr);
	}

	std::optional<int> getUserIdByApiKey(const std::string &apiKey) {
		auto res = simpleQuery("select user_id from api_keys where key = $1", {apiKey});
		if (res.size() == 1) {
			return res[0][0].as<int>();
		} else if (res.size() > 1) {
			throw DbException("Too many results for user by api key!");
		}

		return {};
	}

	std::optional<DbUserInfo> getUserByUid(int uid) {
		auto res = simpleQuery("select login, gid from users where id = $1", {uid});
		if (res.size() == 1) {
			return {{uid, res[0][1].as<int>(), res[0][0].as<std::string>()}};
		} else if (res.size() > 1) {
			throw DbException("Too many results for user by uid!");
		}

		return {};
	}

	std::optional<DbUserInfo> getUserByLoginAndPassword(const std::string &login, const std::string &password) {
		auto res = simpleQuery("select id, login, gid from users where login = $1 and pass = md5($2)", {login, password});
		if (res.size() == 1) {
			return {{res[0][0].as<int>(), res[0][2].as<int>(), res[0][1].as<std::string>()}};
		} else if (res.size() > 1) {
			throw DbException("Too many results for user by login and password!");
		}

		return {};
	}
};

//

class Database {
private:
	bool usePg;
public:
	Database() {
		usePg = config["useDatabase"].asString() == "postgres";
	}

	void reconnect() {
		if (usePg) {
			DatabasePostgres db;
			db.reconnect();
		} else {
			DatabaseMysql db;
			db.reconnect();
		}
	}

	std::optional<int> getUserIdByApiKey(const std::string &apiKey) {
		if (usePg) {
			DatabasePostgres db;
			return db.getUserIdByApiKey(apiKey);
		} else {
			DatabaseMysql db;
			return db.getUserIdByApiKey(apiKey);
		}
	}

	std::optional<DbUserInfo> getUserByUid(int uid) {
		if (usePg) {
			DatabasePostgres db;
			return db.getUserByUid(uid);
		} else {
			DatabaseMysql db;
			return db.getUserByUid(uid);
		}
	}

	std::optional<DbUserInfo> getUserByLoginAndPassword(const std::string &login, const std::string &password) {
		if (usePg) {
			DatabasePostgres db;
			return db.getUserByLoginAndPassword(login, password);
		} else {
			DatabaseMysql db;
			return db.getUserByLoginAndPassword(login, password);
		}
	}
};

#endif

