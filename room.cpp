#include "room.h"

#include <functional>

#include "web_session.h"
#include "lobby.h"

Room::Room(std::string title, const int max, const int number)
{
  title_ = title;
  max_ = max;
  number_ = number;
  session_count_ = 0;
}

int Room::join(const SessionPtr &s)
{
  s->set_room(getPtr());
  sessions_.push_back(s);
  session_count_ = sessions_.size();
  return 1;
}

void Room::leave(const SessionPtr &s)
{
  sessions_.remove_if([&s](const SessionWeakPtr &ws) -> bool {
    auto ss = ws.lock();
    if (ss)
    {
      return s->get_fd() == ss->get_fd();
    }
    return false;
  });

  session_count_ = sessions_.size();

  lwsl_notice("the room has session count: %lu\n", session_count_);

  if (session_count_ < 1)
  {
    auto wl = lobby_.lock();
    if (wl)
    {
      wl->leave(getPtr());
    }
  }
}

void Room::BroadCast(const SessionPtr &me, const std::string &msg)
{
  for (auto s : sessions_)
  {
    auto ws = s.lock();
    if (ws && (ws->get_fd() != me->get_fd()))
    {
      ws->Send(msg.c_str(), msg.size());
    }
  }
}
void Room::BroadCast(const std::string &msg)
{
  BroadCast(msg.c_str(), msg.size());
}

void Room::BroadCast(const char *msg, const size_t len)
{
  for (auto s : sessions_)
  {
    auto ws = s.lock();
    if (ws)
    {
      ws->Send(msg, len);
    }
  }
}

std::string Room::get_object_room_users_info()
{
  json_object *obj = json_object_new_object();
  json_object_object_add(obj, "type", json_object_new_string("room_users_info"));

  json_object *sessions = json_object_new_array();
  for (auto s : sessions_)
  {
    auto ws = s.lock();
    if (ws)
    {
      json_object *userid = json_object_new_object();
      json_object_object_add(userid, "userid", json_object_new_string(ws->get_userid()));
      json_object_array_add(sessions, userid);
    }
  }

  json_object_object_add(obj, "users", sessions);
  const char *sobj = json_object_to_json_string(obj);
  std::string robj(sobj);
  json_object_put(obj);
  return robj;
}

json_object *Room::get_object_room_info_object()
{
  json_object *obj = json_object_new_object();
  json_object_object_add(obj, "type", json_object_new_string("room_info"));
  json_object_object_add(obj, "title", json_object_new_string(title_.c_str()));
  json_object_object_add(obj, "user_count", json_object_new_int(session_count_));

  json_object *sessions = json_object_new_array();
  for (auto s : sessions_)
  {
    auto ws = s.lock();
    if (ws)
    {
      json_object *userid = json_object_new_object();
      json_object_object_add(userid, "userid", json_object_new_string(ws->get_userid()));
      json_object_array_add(sessions, userid);
    }
  }

  json_object_object_add(obj, "users", sessions);
  return obj;
}

std::string Room::get_object_room_info()
{
  json_object *obj = get_object_room_info_object();

  const char *sobj = json_object_to_json_string(obj);
  std::string robj(sobj);
  json_object_put(obj);
  return robj;
}

void Room::set_lobby(const std::shared_ptr<Lobby> &lobby)
{
  lobby_ = lobby;
}