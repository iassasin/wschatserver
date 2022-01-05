#ifndef WSSERVER_SERVER_FWD_HPP
#define WSSERVER_SERVER_FWD_HPP

#include <memory>
#include <jsoncpp/json/json.h>
#include "server_ws_ex.hpp"

class Server;
using WSServerBase = SimpleWeb::SocketServerBase<SimpleWeb::WS>;
using WSServer = WebSocketServerEx;
using ConnectionPtr = std::shared_ptr<WSServerBase::Connection>;

#endif //WSSERVER_SERVER_FWD_HPP
