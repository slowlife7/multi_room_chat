#ifndef _LOBBY_H_
#define _LOBBY_H_

#include <list>
#include <memory>
#include <string>

class WebServer;
class Room;
typedef std::shared_ptr<Room> RoomPtr;

class Lobby {
public:
	void add(RoomPtr room);
	void remove(RoomPtr room);
	void print();

	void update();

  std::string print1();
  
  std::shared_ptr<Room> find(const char *title);
	void set_web_server(WebServer *server);


private:
	std::list<RoomPtr> rooms;
	WebServer *server_;
};

#endif