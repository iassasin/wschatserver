#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <iostream>

#include "../db.hpp"

using namespace std;

int main_old(int argc, char **argv){
	sql::Connection *con;

	sql::mysql::MySQL_Driver driver;
	con = driver.connect("tcp://localhost:3306", "www", "");

	if (!con->isClosed()){
		cout << "Connect ok" << endl;
		auto stmt = con->createStatement();
		
		stmt->execute("use www");
		auto rs = stmt->executeQuery("SELECT id, login FROM users");
		while (rs->next()){
			cout << rs->getInt("id") << " -> " << rs->getString("login") << endl;
		}
		
		delete rs;
		delete stmt;
	}

	delete con;
	return 0;
}

int main(int argc, char **argv){
	Database db;
	auto stmt = db.statement();
	auto rs = stmt->executeQuery("SELECT id, login FROM users ORDER BY id LIMIT 0, 5");
	while (rs->next()){
		cout << rs->getInt("id") << " -> " << rs->getString("login") << endl;
	}
	return 0;
}

