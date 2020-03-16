#ifndef _LOBBY_H_
#define _LOBBY_H_

#include <list>
#include <memory>
#include <string>

class Room;
class Session;
typedef std::shared_ptr<Room> RoomPtr;
typedef std::shared_ptr<Session> SessionPtr;
typedef std::weak_ptr<Session> WeakPtr;
class Lobby : public std::enable_shared_from_this<Lobby>
{
public:
  void join(const SessionPtr &s);
  void leave(const SessionPtr &s);
  void leave(const RoomPtr &r);

  RoomPtr createRoom(const char *title, const char *userid);
  int joinRoom(const char *title, const SessionPtr &s);

  void BroadCast(const SessionPtr& me, const std::string &msg);
  void BroadCast(const std::string &msg);
  void BroadCast(const char *msg, const size_t len);

  std::string get_object_lobby_info();
  std::string get_object_lobby_update();

  std::shared_ptr<Lobby> getPtr()
  {
    return shared_from_this();
  }

private:
  std::list<RoomPtr> rooms_;
  std::list<WeakPtr> sessions_;
  int room_number_;
};

#endif