#include "db.hpp"

unique_ptr<sql::Connection> DatabaseMysql::conn = nullptr;
unique_ptr<pqxx::connection> DatabasePostgres::conn = nullptr;
