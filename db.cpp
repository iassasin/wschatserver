#include "db.hpp"

unique_ptr<sql::Connection> Database::conn = nullptr;
time_t Database::lastReconnectionTime = 0;
