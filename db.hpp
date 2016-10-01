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

using std::string;
using std::unique_ptr;

class Database {
private:
	static unique_ptr<sql::Connection> conn;
	
	void init(){
		execute("set names utf8");
		execute("use www");
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
	
	bool execute(const string &sql){
		return statement()->execute(sql);
	}
	
	unique_ptr<sql::ResultSet> query(const string &sql){
		return unique_ptr<sql::ResultSet>(statement()->executeQuery(sql));
		//TODO: обертка, чтобы сохранять statement()
	}
	
	unique_ptr<sql::Statement> statement(){
		return unique_ptr<sql::Statement>(conn->createStatement());
	}
	
	unique_ptr<sql::PreparedStatement> prepare(const string &sql){
		return unique_ptr<sql::PreparedStatement>(conn->prepareStatement(sql));
	}

};

#endif

