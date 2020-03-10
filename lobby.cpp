

#include <iostream>
#include <algorithm>
#include <functional>
#include <string.h>
#include "room.h"
#include "lobby.h"
#include "web_server.h"
#include "json/json.h"

struct IsTitle {
  IsTitle(const char *title) : title_(title) {}

  bool operator()(std::shared_ptr<Room> room) const {
    return !strcmp(room->get_title(), title_);
  }

  const char *title_;
};

void Lobby::add(RoomPtr room) {
	rooms.push_back(room);
	std::cout << "방 갯수:" << rooms.size() << std::endl;
}

bool remover(int i, int j) {
	return i == j;
}

void Lobby::remove(RoomPtr room) {

	rooms.remove(room);
	std::cout << "남은 방 갯수:" << rooms.size() << std::endl;

  auto s = print1();
  server_->Send((char*)s.c_str(), s.size());
}

void Lobby::update() {
   auto s = print1();
  server_->Send((char*)s.c_str(), s.size());
}

void Lobby::set_web_server(WebServer *server)
{
  server_=server;
}
void Lobby::print()
{
	for (auto r : rooms) {
		r->info();
	}
}

std::string Lobby::print1()
{
  json_object *obj = json_object_new_object();
  json_object_object_add(obj, "cmd", json_object_new_string("room_list"));
  json_object *arr = json_object_new_array();
  for (auto r : rooms) {
		json_object_array_add(arr, r->info1());
	}

  json_object_object_add(obj, "lists", arr);
  const char *sobj = json_object_to_json_string(obj);

  std::string robj(sobj);
  json_object_put(obj);

  return robj;
}


std::shared_ptr<Room> Lobby::find(const char *title)
{
  auto f = std::find_if(rooms.begin(), rooms.end(), IsTitle(title));
  if(f != rooms.end()) {
    return *f;
  }
  return nullptr;
}