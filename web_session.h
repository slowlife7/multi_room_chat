#ifndef _WEB_SESSION_H_
#define _WEB_SESSION_H_

#include <memory>
#include <queue>
#include <vector>

#include <libwebsockets.h>

class Room;
class Lobby;
class User;

typedef std::weak_ptr<Room> RoomWeakPtr;
typedef std::shared_ptr<Room> RoomPtr;
typedef std::weak_ptr<Lobby> LobbyWeakPtr;
typedef std::shared_ptr<Lobby> LobbyPtr;
typedef std::shared_ptr<User> UserPtr;

class Session : public std::enable_shared_from_this<Session>
{
public:
  Session(struct lws *wsi);

  int OnSend();

  int Send(const char *_in, const size_t _len);
  int OnRecv(char *in, size_t len);
  void OnClose();

  void set_fd(const uint32_t fd);
  void set_room(const RoomPtr &room);

  void leave();

  void set_lobby(const LobbyPtr &lobby);

  uint32_t get_fd() const;
  std::shared_ptr<Session> getPtr()
  {
    return shared_from_this();
  }

  void set_userid(const char *userid)
  {
    userid_ = userid;
  }
  const char *get_userid() const
  {
    return userid_.c_str();
  }

private:
  void BroadCastLobbyInfo();
  void BroadCastRoomInfo();

  std::queue<std::vector<char>> sx_buffer_;
  struct lws *wsi_;

  UserPtr user_;
  RoomWeakPtr current_room_;
  LobbyWeakPtr lobby_;
  uint32_t fd_;
  std::string userid_;
};

#endif