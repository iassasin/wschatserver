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
	static time_t lastReconnectionTime;
	
	void init(){
		execute("set names utf8");
		execute("use www");
	}

	bool execute(const string &sql){
		return statement()->execute(sql);
	}

	unique_ptr<sql::ResultSet> query(const string &sql){
		return unique_ptr<sql::ResultSet>(statement()->executeQuery(sql));
		//TODO: обертка, чтобы сохранять statement()
	}
public:
	Database(){
		if (!conn || conn->isClosed() || time(nullptr) - lastReconnectionTime >= 60*60*12){
			reconnect();
		}
	}
	
	void reconnect(){
		sql::mysql::MySQL_Driver driver;
		auto dbconf = config["database"];

		if (conn){
			conn->close();
		}

		conn = unique_ptr<sql::Connection>(driver.connect(dbconf["host"].asString(), dbconf["user"].asString(), dbconf["password"].asString()));
		lastReconnectionTime = time(nullptr);
		init();
	}
	
	SafeStatement<sql::Statement> statement(){
		return SafeStatement<sql::Statement>(*this, conn->createStatement());
	}
	
	SafeStatement<sql::PreparedStatement> prepare(const string &sql){
		return SafeStatement<sql::PreparedStatement>(*this, conn->prepareStatement(sql));
	}

};

template <typename T>
bool SafeStatement<T>::execute(){
	if (pst){
		int tries = 3;
		while (tries--){
			try {
				return pst->execute();
			} catch (sql::SQLException &e){
				if (tries < 0){
					throw;
				}

				Logger::error("SQLException code ", e.getErrorCode(), ", SQLState: ", e.getSQLState(), "\n", e.what());
				db->reconnect();
			}
		}
	}
	return false;
}

template <typename T>
unique_ptr<sql::ResultSet> SafeStatement<T>::executeQuery(){
	if (pst){
		int tries = 3;
		while (tries--){
			try {
				return as_unique(pst->executeQuery());
			} catch (sql::SQLException &e){
				if (tries <= 0){
					throw;
				}

				Logger::error("SQLException code ", e.getErrorCode(), ", SQLState: ", e.getSQLState(), "\n", e.what());
				db->reconnect();
			}
		}
	}
	return nullptr;
}

template <typename T>
int SafeStatement<T>::executeUpdate(){
	if (pst){
		int tries = 3;
		while (tries--){
			try {
				return pst->executeUpdate();
			} catch (sql::SQLException &e){
				if (tries < 0){
					throw;
				}

				Logger::error("SQLException code ", e.getErrorCode(), ", SQLState: ", e.getSQLState(), "\n", e.what());
				db->reconnect();
			}
		}
	}
	return 0;
}

#endif

