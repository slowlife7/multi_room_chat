#include "web_server.h"

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "json/json.h"

#include "web_session.h"
#include "lobby.h"
#include "room.h"

static int interrupted;

struct vhd_handle
{
  struct lws_context *context;
  struct lws_vhost *vhost;
  int *interrupted;
};

static const struct lws_protocol_vhost_options pvo_interrupted = {
    NULL,
    NULL,
    "interrupted",
    (const char *)&interrupted};

static const struct lws_protocol_vhost_options pvo = {
    NULL,
    &pvo_interrupted,
    "web_server",
    ""};

int ServiceCallback(lws *wsi, lws_callback_reasons reason,
                    void *user, void *in, size_t len)
{
  auto web_server = static_cast<WebServer *>(lws_context_user(lws_get_context(wsi)));

  struct vhd_handle *vhd = (struct vhd_handle *)lws_protocol_vh_priv_get(lws_get_vhost(wsi),
                                                                         lws_get_protocol(wsi));

  switch (reason)
  {
  case LWS_CALLBACK_PROTOCOL_INIT:
  {
    lwsl_warn("LWS_CALLBACK_PROTOCOL_INIT\n");
    vhd = (vhd_handle *)lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi), lws_get_protocol(wsi), sizeof(vhd_handle));

    if (!vhd)
      return -1;

    vhd->context = lws_get_context(wsi);
    vhd->vhost = lws_get_vhost(wsi);

    //const struct lws_protocol_vhost_options *ind = (struct lws_protocol_vhost_options *)lws_pvo_search((const struct lws_protocol_vhost_options *)in, "interrupted");
  }
  break;

  case LWS_CALLBACK_ESTABLISHED:
  {
    uint32_t fd = lws_get_socket_fd(wsi);
    lwsl_user("LWS_CALLBACK_ESTABLISHED %u\n", fd);

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    memset(&client, 0x00, sizeof(client));
    int n = getpeername(fd, (struct sockaddr *)&client, &client_len);

    lwsl_user("connected ip :%s port :%u \n", inet_ntoa(client.sin_addr), client.sin_port);

    auto new_session = std::make_shared<Session>(wsi);

    new_session->set_ip(inet_ntoa(client.sin_addr));
    new_session->set_port(client.sin_port);

    web_server->Register(fd, new_session);
  }
  break;

  case LWS_CALLBACK_SERVER_WRITEABLE:
  {
    uint32_t fd = lws_get_socket_fd(wsi);
    lwsl_user("LWS_CALLBACK_WRITEABLE %u\n", fd);
    web_server->OnWrite(fd);
    break;
  }
  case LWS_CALLBACK_RECEIVE:
  {
    uint32_t fd = lws_get_socket_fd(wsi);
    lwsl_user("LWS_CALLBACK_RECEIVE %u\n", fd);
    web_server->OnRecv(fd, static_cast<char *>(in), len);
  }
  break;

  case LWS_CALLBACK_CLOSED:
  {
    uint32_t fd = lws_get_socket_fd(wsi);
    lwsl_user("LWS_CALLBACK_CLOSED %u\n", fd);

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    memset(&client, 0x00, sizeof(client));
    getpeername(fd, (struct sockaddr *)&client, &client_len);

    lwsl_user("connected ip :%s port :%u \n", inet_ntoa(client.sin_addr), client.sin_port);
    web_server->UnRegister(fd);
  }
  break;

  default:
    break;
  }
  return 0;
}

WebServer::WebServer(const size_t rx_buffer) : rx_buffer_(rx_buffer)
{
  lobby_ = std::make_shared<Lobby>();
}
WebServer::~WebServer()
{
  lws_context_destroy(context_);
}

int WebServer::Initialize(const uint32_t port)
{

  int logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE;
  lws_set_log_level(logs, NULL);

  memset(&info_, 0, sizeof(info_));

  struct lws_protocols protocols[] = {
      {"web_server",
       ServiceCallback,
       0,
       rx_buffer_,
       0,
       NULL,
       0},
      {NULL,
       NULL,
       0,
       0}};

  info_.port = port;
  info_.protocols = protocols;
  info_.pt_serv_buf_size = 32 * 1024;
  info_.user = this;
  info_.pvo = &pvo;
  info_.options = LWS_SERVER_OPTION_VALIDATE_UTF8 |
                  LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE | LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

  info_.ssl_cert_filepath = "/home/slowlife/www/cert/cert1.pem";
  info_.ssl_private_key_filepath = "/home/slowlife/www/cert/privkey1.pem";

  context_ = lws_create_context(&info_);
  if (!context_)
  {
    lwsl_err("lws init failed\n");
    return 1;
  }

  lwsl_notice("success initialize\n");

  return 0;
}

void WebServer::Register(uint32_t fd, std::shared_ptr<Session> session)
{
  session->set_fd(fd);
  session->set_lobby(lobby_);
  auto pr = std::make_pair(fd, session);
  sessions_.insert(pr);
  lwsl_notice("register!! %u\n", fd);
}

void WebServer::UnRegister(uint32_t fd)
{
  auto f = sessions_.find(fd);
  if (f != sessions_.end())
  {
    lwsl_notice("unregister!! %u\n", fd);
    f->second->OnClose();
    sessions_.erase(fd);
  }
}

void WebServer::OnRecv(uint32_t fd, char *in, size_t len)
{
  auto f = sessions_.find(fd);
  if (f != sessions_.end())
  {
    lwsl_user("from: %u -> %s(%lu) \n", fd, in, len);
    auto s = f->second;
    if (!PreTranslate(s, in, len))
    {
      return;
    }
    s->OnRecv(in, len);
  }
}

int WebServer::PreTranslate(std::shared_ptr<Session> s, char *_in, size_t len)
{
  json_object *obj = json_tokener_parse(_in);
  if (!obj)
  {
    lwsl_err("json parse is null\n");
    return 0;
  }
  json_object *type = json_object_object_get(obj, "type");
  const char *stype = json_object_get_string(type);

  if (!stype)
    return 0;

  if (!strcmp(stype, "make_room"))
  {
    json_object *title = json_object_object_get(obj, "title");
    const char *stitle = json_object_get_string(title);

    auto room = lobby_->createRoom(stitle, s->get_userid());
    if (room)
    {
      lobby_->leave(s);
      int ret = room->join(s);
      json_object_object_add(obj, "result", json_object_new_int(ret));
    }
    else
    {
      json_object_object_add(obj, "result", json_object_new_int(400));
    }
    const char *out = json_object_to_json_string(obj);
    size_t out_len = strlen(out);
    s->Send(out, out_len);
    json_object_put(obj);
  }
  else if (!strcmp(stype, "join_room"))
  {
    json_object *title = json_object_object_get(obj, "title");
    const char *stitle = json_object_get_string(title);

    lobby_->joinRoom(stitle, s);
  }
  else if (!strcmp(stype, "leave_room"))
  {
    lobby_->join(s);
    json_object_object_add(obj, "result", json_object_new_int(1));
    const char *out = json_object_to_json_string(obj);
    size_t out_len = strlen(out);
    s->Send(out, out_len);
    json_object_put(obj);
  }
  return 1;
}

void WebServer::Send(uint32_t fd, char *_in, size_t len)
{
  for (auto s : sessions_)
  {
    if (s.second->get_fd() != fd)
      s.second->Send(_in, len);
  }
}

void WebServer::Send(char *_in, size_t len)
{
  for (auto s : sessions_)
  {
    s.second->Send(_in, len);
  }
}

void WebServer::OnWrite(uint32_t fd)
{
  auto f = sessions_.find(fd);
  if (f != sessions_.end())
  {
    f->second->OnSend();
  }
}

int WebServer::Run()
{
  lwsl_notice("service start\n");
  int n = 0;
  while (n >= 0)
  {
    n = lws_service(context_, 0);
  }
  lwsl_notice("service finished\n");
  return 0;
}
