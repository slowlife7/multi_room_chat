#ifndef _ROOM_H_
#define _ROOM_H_

#include <list>
#include <string>
#include <memory>
#include <iostream>
#include "json/json.h"

class Session;
class Lobby;

typedef std::weak_ptr<Lobby> LobbyWeakPtr;
typedef std::shared_ptr<Lobby> LobbyPtr;
typedef std::shared_ptr<Session> SessionPtr;
typedef std::weak_ptr<Session> SessionWeakPtr;

class Room : public std::enable_shared_from_this<Room> {
public:
	Room(LobbyPtr lobby, std::string title, const int max, const int number ) {
    lobby_=lobby;
    title_ = title;
    max_ = max;
    number_ = number;
  }

	void join(SessionPtr user);
	void leave(SessionPtr user);
	void send(std::string from, std::string msg);
	void notify(std::string msg);

	std::shared_ptr<Room> getPtr() {
		return shared_from_this();
	}

  const char * get_title() const {
    return title_.c_str();
  }

	void info() {
		std::cout << "=====================================================" << std::endl;
		std::cout << "[" << number_ << "]" << title_ << "-" << "(" << users.size() << "/" << max_ << ")" << std::endl;
		std::cout << "=====================================================" << std::endl;
	}

  json_object* info1() {
    json_object *obj = json_object_new_object();
    json_object_object_add(obj, "number", json_object_new_int(number_));
    json_object_object_add(obj, "title", json_object_new_string(title_.c_str()));
    json_object_object_add(obj, "user_count", json_object_new_int(users.size()));
    json_object_object_add(obj, "max_count", json_object_new_int(max_));
    return obj;
  }

  std::string info2();
private:
	 int number_;
	 int max_;
	 std::string title_;

	LobbyWeakPtr lobby_;
	std::list<SessionPtr> users;
};

#endif