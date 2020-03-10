#ifndef _WEB_SESSION_H_
#define _WEB_SESSION_H_

#include <memory>
#include <queue>
#include <vector>
#include <libwebsockets.h>

class Room;
typedef std::weak_ptr<Room> RoomWeakPtr;
typedef std::shared_ptr<Room> RoomPtr;

class WebServer;
class Session : public std::enable_shared_from_this<Session>
{
public:
  Session(struct lws *wsi);

  int OnSend();

  int Send(const char *_in, const size_t _len);

  int OnRecv(char *in, size_t len);
  int OnRecvSelf(char *in, size_t len);
  int OnRecvRoom(char *in, size_t len);

  void OnClose();

  void SetWebServer(WebServer *server);

  void set_fd(const uint32_t fd);
  uint32_t get_fd() const;
  void Join(RoomPtr& room);
  void Leave();
  //void Send(std::string msg);
  void SendRoom(std::string msg);
  //void Recv(std::string from, std::string msg);
  void Notify(std::string msg);
   void set_userid(std::string userid);
	std::shared_ptr<Session> getPtr() {
		return shared_from_this();
	}

  std::string getUserid();


private:
  std::queue<std::vector<char>> sx_buffer_;
  struct lws *wsi_;
  uint32_t    fd_;
  WebServer *server_;

  std::string userid_;
  RoomWeakPtr room_;
};

#endif