//
// Created by assasin on 17.06.17.
//

#pragma once

#include "simple_wss/server_wss.hpp"
#include "algo.hpp"
#include "logger.hpp"

class WebSocketServerEx : public SimpleWeb::SocketServer<SimpleWeb::WS> {
public:
	WebSocketServerEx() : SimpleWeb::SocketServer<SimpleWeb::WS>() {
		io_service = std::make_shared<boost::asio::io_service>();
		internal_io_service = true;
	}

	void runWithTimeout(int msec, std::function<void()> func) {
		using namespace boost;
		using namespace boost::asio;

		auto timer = std::make_shared<deadline_timer>(*io_service);
		std::function<void(const system::error_code& ec)> f;
		f = [func, timer](const system::error_code& ec) {
			if(!ec) {
				func();
			}
			else {
				Logger::error("Timer error ", ec.value(), ": ", ec.message());
			}
		};

		timer->expires_from_now(posix_time::millisec(msec));
		timer->async_wait(f);
	}

	void runWithInterval(int msec, std::function<void()> func) {
		using namespace boost;
		using namespace boost::asio;

		auto timer = std::make_shared<deadline_timer>(*io_service);
		auto f = std::make_shared<std::function<void(const system::error_code& ec)>>();
		*f = [=](const system::error_code& ec) {
			if(!ec) {
				func();
				timer->expires_from_now(posix_time::millisec(msec));
				timer->async_wait(*f);
			}
			else {
				Logger::error("Timer error ", ec.value(), ": ", ec.message());
			}
		};

		timer->expires_from_now(posix_time::millisec(msec));
		timer->async_wait(*f);
	}

};
