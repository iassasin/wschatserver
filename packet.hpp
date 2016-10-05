#ifndef PACKET_H_
#define PACKET_H_

#include <string>
#include <jsoncpp/json/json.h>

using std::string;

#ifndef CLIENT_CLASS_DEFINED
class Client;
#endif

class Packet {
private:
	
public:
	enum class Type : int { bad=0, system, message, online_list, auth, status, join, leave };
	
	Type type;
	
	Packet();
	virtual ~Packet();
	
	static Packet *read(const string &data);
	
	virtual void deserialize(const Json::Value &) = 0;
	virtual Json::Value serialize() const = 0;
	virtual void process(Client &) = 0;
};

#endif

