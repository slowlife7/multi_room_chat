#ifndef _USER_H_
#define _USER_H_

#include <string>
#include <memory>

class Session;
class Room;
typedef std::weak_ptr<Room> RoomWeakPtr;
typedef std::shared_ptr<Room> RoomPtr;
typedef std::weak_ptr<Session> SessionWeakPtr;

class User : public std::enable_shared_from_this<User> {
public:
	User(const char* userid);
	void join(RoomPtr& room);
	
	void leave();
	
	void send(std::string msg);
	void recv(std::string from, std::string msg);
	void notify(std::string msg);

	void set_session(std::shared_ptr<Session>& session);
	std::string getUserid() ;

	std::shared_ptr<User> getPtr() {
		return shared_from_this();
	}

private:
	RoomWeakPtr room_;
  SessionWeakPtr session_;
	std::string userid_;
};


#endif