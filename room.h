#ifndef _ROOM_H_
#define _ROOM_H_

#include <list>
#include <string>
#include <memory>

#include "json/json.h"

class Session;
class Lobby;
typedef std::weak_ptr<Session> SessionWeakPtr;
typedef std::shared_ptr<Session> SessionPtr;
typedef std::weak_ptr<Lobby> LobbyWeakPtr;
class Room : public std::enable_shared_from_this<Room>
{
public:
  Room(std::string title, const int max, const int number);

  int join(const SessionPtr &s);
  void leave(const SessionPtr &s);

  void BroadCast(const SessionPtr &me, const std::string &msg);
  void BroadCast(const std::string &msg);
  void BroadCast(const char *msg, const size_t len);

  json_object *get_object_room_info_object();
  std::string get_object_room_info();
  std::string get_object_room_users_info();

  std::shared_ptr<Room> getPtr()
  {
    return shared_from_this();
  }

  const char *get_title() const
  {
    return title_.c_str();
  }

  void set_lobby(const std::shared_ptr<Lobby> &lobby);

private:
  int number_;
  int max_;
  std::string title_;
  size_t session_count_;

  LobbyWeakPtr lobby_;
  std::list<SessionWeakPtr> sessions_;
};

#endif