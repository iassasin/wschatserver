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

#include "config.hpp"
#include "algo.hpp"

using std::string;
using std::unique_ptr;

template <typename T>
class SafeStatement {
private:
	//using T = sql::PreparedStatement;
	unique_ptr<T> pst;
public:
	SafeStatement(SafeStatement<T> &) = delete;
	SafeStatement(const SafeStatement<T> &) = delete;

	explicit SafeStatement(T *st){
		pst.reset(st);
	}

	SafeStatement(SafeStatement &&ss){
		pst.swap(ss.pst);
	}

	SafeStatement<T> &operator = (SafeStatement<T> &&ss){
		pst.swap(ss.pst);
		return *this;
	}

	bool execute(){
		return pst && pst->execute();
	}

	unique_ptr<sql::ResultSet> executeQuery(){
		return pst ? as_unique(pst->executeQuery()) : nullptr;
	}

	int executeUpdate(){
		return pst ? pst->executeUpdate() : 0;
	}

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
		if (!conn || conn->isClosed()){
			reconnect();
		}
	}
	
	void reconnect(){
		sql::mysql::MySQL_Driver driver;
		auto dbconf = config["database"];
		conn = unique_ptr<sql::Connection>(driver.connect(dbconf["host"].asString(), dbconf["user"].asString(), dbconf["password"].asString()));
		init();
	}
	
	SafeStatement<sql::Statement> statement(){
		return SafeStatement<sql::Statement>(conn->createStatement());
	}
	
	SafeStatement<sql::PreparedStatement> prepare(const string &sql){
		return SafeStatement<sql::PreparedStatement>(conn->prepareStatement(sql));
	}

};

#endif

