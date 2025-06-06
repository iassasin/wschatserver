cmake_minimum_required(VERSION 3.13)
project(wsserver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

if (NOT ${CMAKE_BUILD_TYPE} STREQUAL Debug)
	set(CMAKE_CXX_FLAGS "-flto ${CMAKE_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "-flto -s ${CMAKE_EXE_LINKER_FLAGS}")
endif ()

include_directories(redisclient/src)

# weird hacks for compiler incompatibilities
include_directories(mysql_cppconn)
include_directories(pqxx)

set(SOURCE_FILES
		simple_wss/server_ws.hpp
		simple_wss/server_wss.hpp
		simple_wss/crypto.hpp

		regex/regex.cpp
		algo.cpp
		config.cpp
		db.cpp
		redis.cpp
		packet.cpp
		packets.cpp
		rooms.cpp
		client.cpp
		server.cpp
		wsserver.cpp
		server_ws_ex.hpp
		commands/command.hpp
		commands/commands.hpp
		commands/command_nick.hpp
		commands/command_gender.hpp
		commands/command_color.hpp
		commands/command_styled_message.hpp
		commands/command_private_message.hpp
		commands/command_moder.hpp
		commands/command_kick.hpp
		commands/command_ban.hpp
		commands/command_userlist.hpp
		commands/command_roomlist.hpp
		commands/command_myroomlist.hpp
		commands/command_ipcounter.hpp
		commands/command_help.hpp
		logger.hpp
		gate.hpp
		utils.hpp
		rooms_fwd.hpp
		client_fwd.hpp
		src/exceptions.hpp src/clients_manager.hpp server_fwd.hpp)

find_package(Boost COMPONENTS system regex REQUIRED)
find_library(PQXX pqxx)
find_library(PQ pq)
find_library(MYSQLCPPCONN mysqlcppconn)
find_library(CRYPTO crypto)
find_library(JSONCPP jsoncpp)
find_library(SSL ssl)

add_executable(wsserver ${SOURCE_FILES})
target_link_libraries(wsserver pthread Boost::system Boost::regex ${MYSQLCPPCONN} ${CRYPTO} ${JSONCPP} ${SSL})
target_link_libraries(wsserver pthread Boost::system Boost::regex ${MYSQLCPPCONN} ${PQXX} ${PQ} ${CRYPTO} ${JSONCPP} ${SSL})