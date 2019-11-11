#ifndef DB_H_
#define DB_H_

#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>
#include <memory>
#include <string>
#include <ctime>

#include "config.hpp"
#include "algo.hpp"
#include "logger.hpp"

using std::string;
using std::unique_ptr;

class Database;

template <typename T>
class SafeStatement {
private:
	Database *db;
	unique_ptr<T> pst;

	template<typename F, typename R>
	R retryWithReconnect(int count, R def, F f);
public:
	SafeStatement(SafeStatement<T> &) = delete;
	SafeStatement(const SafeStatement<T> &) = delete;

	explicit SafeStatement(Database &fdb, T *st){
		pst.reset(st);
		db = &fdb;
	}

	SafeStatement(SafeStatement &&ss){
		pst.swap(ss.pst);
		db = ss.db;
	}

	SafeStatement<T> &operator = (SafeStatement<T> &&ss){
		pst.swap(ss.pst);
		db = ss.db;
		return *this;
	}

	bool execute();
	unique_ptr<sql::ResultSet> executeQuery();
	int executeUpdate();

	T *operator -> (){
		return pst.get();
	}

	T &operator * (){
		return *pst;
	}
};

class Database {
private:
	static unique_ptr<sql::Connection> conn;

	bool execute(const string &sql){
		return statement()->execute(sql);
	}

	unique_ptr<sql::ResultSet> query(const string &sql){
		return unique_ptr<sql::ResultSet>(statement()->executeQuery(sql));
		//TODO: обертка, чтобы сохранять statement()
	}
public:
	Database(){
		if (!conn || conn->isClosed()){
			reconnect();
		}
	}
	
	void reconnect(){
		sql::mysql::MySQL_Driver driver;
		auto dbconf = config["database"];

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
	
	SafeStatement<sql::Statement> statement(){
		return SafeStatement<sql::Statement>(*this, conn->createStatement());
	}
	
	SafeStatement<sql::PreparedStatement> prepare(const string &sql){
		return SafeStatement<sql::PreparedStatement>(*this, conn->prepareStatement(sql));
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

				Logger::error("SQLException code ", e.getErrorCode(), ", SQLState: ", e.getSQLState(), "\n", e.what());
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
int SafeStatement<T>::executeUpdate(){
	return retryWithReconnect(3, 0, [this] {
		return pst->executeUpdate();
	});
}

#endif

