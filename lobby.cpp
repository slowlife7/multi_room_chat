#include "lobby.h"

#include <iostream>
#include <algorithm>
#include <functional>
#include <string.h>

#include "json/json.h"

#include "room.h"
#include "web_session.h"

struct IsTitle
{
  IsTitle(const char *title) : title_(title) {}

  bool operator()(std::shared_ptr<Room> room) const
  {
    return !strcmp(room->get_title(), title_);
  }

  const char *title_;
};

void Lobby::join(const SessionPtr &s)
{
  s->leave();
  sessions_.push_back(s);

  auto res = get_object_lobby_info();
  s->Send(res.c_str(), res.size());

  auto li = get_object_lobby_update();
  BroadCast(li);
}

void Lobby::leave(const SessionPtr &s)
{
  uint32_t fd = s->get_fd();

  sessions_.remove_if([&](WeakPtr &item) -> bool {
    auto i = item.lock();
    if (i)
    {
      return fd == i->get_fd();
    }
    return false;
  });
}

void Lobby::leave(const RoomPtr &r)
{
  rooms_.remove(r);

  lwsl_notice("the lobby has room count:%lu\n", rooms_.size());
}

RoomPtr Lobby::createRoom(const char *title, const char *userid)
{
  std::string r = title;
  auto room = std::make_shared<Room>(r, 8, room_number_);
  room->set_lobby(getPtr());
  rooms_.push_back(room);
  return room;
}

int Lobby::joinRoom(const char *title, const SessionPtr &s)
{
  auto f = std::find_if(rooms_.begin(), rooms_.end(), IsTitle(title));
  if (f != rooms_.end())
  {
    leave(s);
    (*f)->set_lobby(getPtr());
    (*f)->join(s);
    return 1;
  }
  return 0;
}

void Lobby::BroadCast(const SessionPtr& me, const std::string &msg)
{
for (auto ws : sessions_)
  {
    auto s = ws.lock();
    if (s && ( s->get_fd() != me->get_fd()))
    {
      s->Send((char *)msg.c_str(), msg.size());
    }
  }
}

void Lobby::BroadCast(const std::string &msg)
{
  for (auto ws : sessions_)
  {
    auto s = ws.lock();
    if (s)
    {
      s->Send((char *)msg.c_str(), msg.size());
    }
  }
}

void Lobby::BroadCast(const char *msg, const size_t len)
{
  for (auto ws : sessions_)
  {
    auto s = ws.lock();
    if (s)
    {
      s->Send((char *)msg, len);
    }
  }
}

std::string Lobby::get_object_lobby_update()
{
   json_object *obj = json_object_new_object();
  json_object_object_add(obj, "type", json_object_new_string("lobby_update"));
  json_object *rooms = json_object_new_array();
  json_object *users = json_object_new_array();

  for (auto r : rooms_)
  {
    json_object_array_add(rooms, r->get_object_room_info_object());
  }

  json_object_object_add(obj, "rooms", rooms);

  for (auto ws : sessions_)
  {
    auto s = ws.lock();
    if (s)
    {
      json_object_array_add(users, json_object_new_string(s->get_userid()));
    }
  }

  json_object_object_add(obj, "users", users);

  const char *sobj = json_object_to_json_string(obj);

  std::string robj(sobj);
  json_object_put(obj);

  return robj;
}

std::string Lobby::get_object_lobby_info()
{
  json_object *obj = json_object_new_object();
  json_object_object_add(obj, "type", json_object_new_string("lobby_info"));
  json_object *rooms = json_object_new_array();
  json_object *users = json_object_new_array();

  for (auto r : rooms_)
  {
    json_object_array_add(rooms, r->get_object_room_info_object());
  }

  json_object_object_add(obj, "rooms", rooms);

  for (auto ws : sessions_)
  {
    auto s = ws.lock();
    if (s)
    {
      json_object_array_add(users, json_object_new_string(s->get_userid()));
    }
  }

  json_object_object_add(obj, "users", users);

  const char *sobj = json_object_to_json_string(obj);

  std::string robj(sobj);
  json_object_put(obj);

  return robj;
}
