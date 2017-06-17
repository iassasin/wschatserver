//
// Created by assasin on 17.06.17.
//

#pragma once

#include "server_wss.hpp"
#include "algo.hpp"

class WebSocketServerEx : public SimpleWeb::SocketServer<SimpleWeb::WSS> {
public:
	WebSocketServerEx(unsigned short port, size_t num_threads, const std::string& cert_file, const std::string& private_key_file,
	            size_t timeout_request=5, size_t timeout_idle=0,
	            const std::string& verify_file=std::string()) :
	            SimpleWeb::SocketServer<SimpleWeb::WSS>(port, num_threads, cert_file, private_key_file, timeout_request, timeout_idle, verify_file)
	{

	}

	void runWithTimeout(int msec, std::function<void()> func){
		using namespace boost;
		using namespace boost::asio;

		auto timer = std::make_shared<deadline_timer>(io_service);
		std::function<void(const system::error_code& ec)> f;
		f = [func, timer](const system::error_code& ec){
			if(!ec){
				func();
			}
			else {
				std::cout << date("[%H:%M:%S] ") << "Server: Timer error " << ec.value() << ": " << ec.message() << std::endl;
			}
		};

		timer->expires_from_now(posix_time::millisec(msec));
		timer->async_wait(f);
	}

	void runWithInterval(int msec, std::function<void()> func){
		using namespace boost;
		using namespace boost::asio;

		auto timer = std::make_shared<deadline_timer>(io_service);
		auto f = std::make_shared<std::function<void(const system::error_code& ec)>>();
		*f = [=](const system::error_code& ec){
			if(!ec){
				func();
				timer->expires_from_now(posix_time::millisec(msec));
				timer->async_wait(*f);
			}
			else {
				std::cout << date("[%H:%M:%S] ") << "Server: Timer error " << ec.value() << ": " << ec.message() << std::endl;
			}
		};

		timer->expires_from_now(posix_time::millisec(msec));
		timer->async_wait(*f);
	}

};
