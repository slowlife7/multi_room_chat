#include "web_session.h"

#include "json/json.h"

#include "user.h"
#include "lobby.h"
#include "room.h"

Session::Session(struct lws *wsi) : wsi_(wsi)
{
  mongoc_init();

  client_ = mongoc_client_new("mongodb://localhost:27017/?appname=multi_room_chat");
  collection_ = mongoc_client_get_collection(client_, "multi_chat", "connection");
}

Session::~Session()
{
  mongoc_collection_destroy(collection_);
  mongoc_client_destroy(client_);
  mongoc_cleanup();
}

int Session::OnSend()
{

  if (sx_buffer_.empty())
    return 0;

  auto &m = sx_buffer_.front();
  int len = m.size();

  //lwsl_notice("on_send m:%s l:%d\n", (char*)m.data(), len);

  lws_write(wsi_, ((unsigned char *)m.data()) + LWS_PRE, len - LWS_PRE, LWS_WRITE_TEXT);
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
  s.resize(LWS_PRE + _len);

  lwsl_notice("to: %u, %s(%lu)\n", fd_, _in, _len);
  memcpy(s.data() + LWS_PRE, _in, _len);

  sx_buffer_.push(s);
  lws_callback_on_writable(wsi_);
  return 1;
}

int Session::OnRecv(char *in, size_t len)
{
  json_object *obj = json_tokener_parse(in);
  json_object *type = json_object_object_get(obj, "type");
  const char *stype = json_object_get_string(type);

  if (!strcmp(stype, "create_user"))
  {
    json_object *userid = json_object_object_get(obj, "userid");
    const char *suserid = json_object_get_string(userid);

    user_ = std::make_shared<User>();
    set_userid(suserid);
    auto wl = lobby_.lock();
    if (wl)
    {
      wl->join(this->getPtr());
    }

    bson_oid_init(&oid, NULL);

    struct tm t = {0};

    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long millisecondsSinceEpoch =
        (unsigned long long)(tv.tv_sec) * 1000 +
        (unsigned long long)(tv.tv_usec) / 1000;
    bson_t *doc = BCON_NEW(
        "_id", BCON_OID(&oid),
        "create_time", BCON_DATE_TIME(millisecondsSinceEpoch),
        "leave_time", BCON_DATE_TIME(millisecondsSinceEpoch),
        "userid", BCON_UTF8(get_userid()),
        "ip", BCON_UTF8(get_ip()),
        "port", BCON_INT32(get_port()));
    bson_error_t error;
    if (!mongoc_collection_insert_one(
            collection_, doc, NULL, NULL, &error))
    {
      lwsl_warn("%s\n", error.message);
    }
    bson_destroy(doc);
    //logged
  }
  else if (!strcmp(stype, "make_room"))
  {
    auto wr = current_room_.lock();
    if (wr)
    {
      auto info = wr->get_object_room_info();
      Send((char *)info.c_str(), info.size());
    }
    BroadCastLobbyInfo();
    BroadCastRoomInfo();
  }
  else if (!strcmp(stype, "join_room"))
  {
    auto wr = current_room_.lock();
    if (wr)
    {
      auto info = wr->get_object_room_info();
      Send((char *)info.c_str(), info.size());
    }
    BroadCastLobbyInfo();
    BroadCastRoomInfo();
  }
  else if (!strcmp(stype, "leave_room"))
  {
    BroadCastLobbyInfo();
    BroadCastRoomInfo();
  }
  else if (!strcmp(stype, "room_chat"))
  {
    const char *sobj = json_object_to_json_string(obj);
    size_t sobj_len = strlen(sobj);
    auto wr = current_room_.lock();
    if (wr)
    {
      wr->BroadCast(sobj, sobj_len);
    }
  }
  else if (!strcmp(stype, "lobby_chat"))
  {
    const char *sobj = json_object_to_json_string(obj);
    size_t sobj_len = strlen(sobj);
    auto wl = lobby_.lock();
    if (wl)
    {
      wl->BroadCast(sobj, sobj_len);
    }
  }

  lws_callback_on_writable(wsi_);
  return 1;
}

void Session::BroadCastLobbyInfo()
{
  auto wl = lobby_.lock();
  if (wl)
  {
    auto lobby_info = wl->get_object_lobby_update();
    wl->BroadCast(getPtr(), lobby_info);
  }
}
void Session::BroadCastRoomInfo()
{
  auto wr = current_room_.lock();
  if (wr)
  {
    auto room_info = wr->get_object_room_users_info();
    wr->BroadCast(getPtr(), room_info);
  }
}
void Session::OnClose()
{
  auto wr = current_room_.lock();
  if (wr)
  {
    wr->leave(this->getPtr());
    BroadCastRoomInfo();
  }

  auto wl = lobby_.lock();
  if (wl)
  {
    wl->leave(this->getPtr());
    BroadCastLobbyInfo();
  }
  struct timeval tv;
  gettimeofday(&tv, NULL);
  unsigned long long millisecondsSinceEpoch =
      (unsigned long long)(tv.tv_sec) * 1000 +
      (unsigned long long)(tv.tv_usec) / 1000;
  bson_t *query = BCON_NEW("_id", BCON_OID(&oid));
  bson_t *update = BCON_NEW("$set",
                            "{",
                            "leave_time",
                            BCON_DATE_TIME(millisecondsSinceEpoch),
                            "updated",
                            BCON_BOOL(true),
                            "}");
  bson_error_t error;
  if (!mongoc_collection_update_one(
          collection_, query, update, NULL, NULL, &error))
  {
    lwsl_warn("%s\n", error.message);
  }

  bson_destroy(query);
  bson_destroy(update);

  //logged mongodb
}

void Session::set_fd(const uint32_t fd)
{
  fd_ = fd;
}

void Session::set_room(const RoomPtr &room)
{
  current_room_ = room;
}
void Session::set_lobby(const LobbyPtr &lobby)
{
  lobby_ = lobby;
}

uint32_t Session::get_fd() const
{
  return fd_;
}
void Session::leave()
{
  auto wr = current_room_.lock();
  if (wr)
  {
    wr->leave(getPtr());
  }
}