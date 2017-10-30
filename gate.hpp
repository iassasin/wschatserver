//
// Created by assasin on 30.10.17.
//

#ifndef WSSERVER_GATE_HPP
#define WSSERVER_GATE_HPP

#include "memcached.hpp"

#include <string>

class Gate {
protected:
	Memcache md;

	bool _tries(const std::string &name, const std::string &key, int tries, int timeout){
		string mdkey = "gate-" + name + "-" + key;
		int cnt;

		if (!md.getPhp(mdkey, cnt)){
			cnt = 0;
		}

		++cnt;
		if (cnt > tries){
			return false;
		}
		md.setPhp(mdkey, cnt, timeout);

		return true;
	}

	void _resetTries(const std::string &name, const std::string &key){
		string mdkey = "gate-" + name + "-" + key;
		int cnt = 0;
		if (md.getPhp(mdkey, cnt) && cnt > 0){
			md.setPhp(mdkey, 0, 1);
		}
	}
public:
	bool auth(const string &ip, bool reset = false){
		if (reset){
			_resetTries("auth", ip);
			return true;
		}
		return _tries("auth", ip, 4, 5*60);
	}
};

#endif //WSSERVER_GATE_HPP
