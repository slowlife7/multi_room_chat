#include "User.h"

#include <iostream>

#include "Room.h"

User::User( char* userid) {

  userid_ = userid;
}

void User::join(RoomPtr& room) {
	room_ = room;
	RoomPtr roomPtr = room_.lock();
	if (roomPtr)
	{
		roomPtr->join(this->getPtr());
	}
}
void User::leave() {
	RoomPtr roomPtr = room_.lock();
	if (roomPtr)
	{
		roomPtr->leave(this->getPtr());
	}
}
void User::send(std::string msg) {
	RoomPtr roomPtr = room_.lock();
	if (roomPtr)
	{
		roomPtr->send(userid_, msg);
	}
}
void User::recv(std::string from, std::string msg) {
	std::cout << "["<< from <<"] "<<msg << std::endl;
}
void User::notify(std::string msg)
{
	std::cout << msg << std::endl;
}

std::string User::getUserid()  {
	return userid_;
}

void User::set_session(std::shared_ptr<Session>& session)
{
  session_ = session;
} 

