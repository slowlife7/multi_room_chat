

#include "room.h"
#include "web_server.h"
#include "web_session.h"

Session::Session(struct lws *wsi) : wsi_(wsi), userid_("") {}

int Session::OnSend()
{

  if (sx_buffer_.empty())
    return 0;

  auto &m = sx_buffer_.front();
  int len = m.size();

  lwsl_user("m:%s l:%d\n", m.data(), len);

  lws_write(wsi_, ((unsigned char *)m.data()) + LWS_PRE, len-LWS_PRE, LWS_WRITE_TEXT);
  sx_buffer_.pop();

  if (!sx_buffer_.empty())
  {
    lws_callback_on_writable(wsi_);
  }
  return len;
}

int Session::Send(const char *_in, const size_t _len)
{

  std::vector<char> s;
  s.resize(LWS_PRE+_len);

  memcpy(s.data()+LWS_PRE, _in, _len);

  sx_buffer_.push(s);
  lws_callback_on_writable(wsi_);
  return 1;
}

int Session::OnRecv(char *in, size_t len)
{
  lwsl_user("%s:\n", in);

  //Send(in, len);
  server_->Send(fd_, in, len);

  lws_callback_on_writable(wsi_);
  return 1;
}

int Session::OnRecvSelf(char *in, size_t len)
{
  lwsl_user("recvself:%s\n", in);
  Send(in, len);
  lws_callback_on_writable(wsi_);
  return 1;
}

int Session::OnRecvRoom(char *in, size_t len)
{
  auto r = room_.lock();
  if(r) {
    r->send("", std::string(in));
  }
  lws_callback_on_writable(wsi_);
  return 1;
}

void Session::OnClose()
{
  Leave();
}

void Session::SetWebServer(WebServer *server)
{
  server_ = server;
}

void Session::set_fd(const  uint32_t fd) 
{
  fd_= fd;
}

uint32_t Session::get_fd() const {
  return fd_;
}

 void Session::Join(RoomPtr& room)
 {
    room_ = room;
	RoomPtr roomPtr = room_.lock();
	if (roomPtr)
	{
		roomPtr->join(this->getPtr());
	}
 }
  void Session::Leave()
  {
    RoomPtr roomPtr = room_.lock();
    lwsl_user("room leave!!!\n");
    if (roomPtr)
    {
      lwsl_user("room leave!!!@@@@@@@\n");
      roomPtr->leave(this->getPtr());
    }
    }

  void Session::SendRoom(std::string msg)
  { 
    RoomPtr roomPtr = room_.lock();
    if (roomPtr)
    {
      roomPtr->send(userid_, msg);
    }
  }
  void Session::Notify(std::string msg)
  {

  }

  void Session::set_userid(std::string userid)
  {
    userid_ = userid;
  }
  std::string Session::getUserid()
  {
    return userid_;
  }