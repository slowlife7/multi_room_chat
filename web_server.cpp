#include <iostream>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include "web_session.h"
#include "web_server.h"
#include "lobby.h"
#include "room.h"
#include "json/json.h"

static int interrupted;

struct per_session_data
{
  int fd;
};

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
    lwsl_user("LWS_CALLBACK_ESTABLISHED %u\n",fd);
    auto new_session = std::make_shared<Session>(wsi);
    
    new_session->SetWebServer(web_server);
    //new_setssion->set_lobby(WebServer->get_lobby());
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
    lwsl_user("LWS_CALLBACK_CLOSED %u\n",fd);
    web_server->UnRegister(fd);
  }
  break;

  default:
    break;
  }
  return 0;
}

WebServer::WebServer(const size_t rx_buffer) : rx_buffer_(rx_buffer) {
  lobby_ = std::make_shared<Lobby>();
  lobby_->set_web_server(this);
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
                  LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

  context_ = lws_create_context(&info_);
  if (!context_)
  {
    lwsl_err("lws init failed\n");
    return 1;
  }

  lwsl_user("success initialize\n");

  return 0;
}

void WebServer::Register(uint32_t fd, std::shared_ptr<Session> session)
{
  session->set_fd(fd);
  auto pr = std::make_pair(fd, session);
  sessions_.insert(pr);
}

void WebServer::UnRegister(uint32_t fd)
{
  auto f = sessions_.find(fd);
  if (f != sessions_.end())
  {
    lwsl_user("unregister!! %u\n", fd);
    f->second->OnClose();
    sessions_.erase(fd);
  }
}


void WebServer::OnRecv(uint32_t fd, char *in, size_t len)
{
  auto f = sessions_.find(fd);
  if (f != sessions_.end())
  {
    auto s = f->second;
    PreTranslate(s, in, len);

    //f->second->OnRecv(in, len);
  }
}

int WebServer::PreTranslate(std::shared_ptr<Session> s, char* _in, size_t len)
{
  json_object *obj = json_tokener_parse(_in);

  json_object *cmd = json_object_object_get(obj, "cmd");
  const char *scmd = json_object_get_string(cmd);

  lwsl_user("cmd :%s\n", scmd);

  int result = 0;
  if(!strcmp(scmd, "create_user")) {
     json_object *user = json_object_object_get(obj, "userid");
     const char *suer = json_object_get_string(user);
     s->set_userid(suer);
     result = 200;

    json_object_object_add(obj, "result", json_object_new_int(result));
    const char *r = json_object_to_json_string(obj);
    size_t l = (size_t)strlen(r);
    s->OnRecvSelf((char*)r, l);

    auto room_info = lobby_->print1();
    s->OnRecvSelf((char*)room_info.c_str(), room_info.size());

    json_object_put(obj);

    /*
    const char* rs = lobby_->get_rooms();
      if(rs) {
        s->OnResponse(rs, strlen(rs));
      }
    */

  } else if(!strcmp(scmd, "create_room")) {
     json_object *title = json_object_object_get(obj, "title");
     json_object *owned = json_object_object_get(obj, "owned");
     json_object *password = json_object_object_get(obj, "password");

    const char *stitle = json_object_get_string(title);
    lwsl_user("title:%s\n", stitle);

    //auto r = lobby_->CreateRoom(obj);
    //if(r) {
    // s->join(r);
    //  json_object_object_add(obj, "result", json_object_new_int(200));
    //   const char* res = json_object_to_json_string(obj);
    //   size_t len = (size_t)strlen(res);
    //   s->OnResponse(res, len);
    //   const char* room_info = r->get_room_info();
    //   s->OnNotifyRoom(room_info, (size_t)strlen(room_info));)
    /*
        const char* rs = lobby_->get_rooms();
        if(rs) {
        s->OnNotifyAllSessions(rs, strlen(rs));
        }
    */
    // } else {
    //   json_object_object_add(obj, "result", json_object_new_int(400));
    //   const char* res = json_object_to_json_string(obj);
    //   size_t len = (size_t)strlen(res);
    //   s->OnResponse(res, len);
    //}

    auto r = std::make_shared<Room>(lobby_, std::string(stitle), 8, 1);
    s->Join(r);
    lobby_->add(r);
    lobby_->print();
    result = 200;
    json_object_object_add(obj, "user_count", json_object_new_int(1));
    json_object_object_add(obj, "max_count", json_object_new_int(8));
    json_object_object_add(obj, "result", json_object_new_int(result));
    const char *r1 = json_object_to_json_string(obj);
    size_t l = (size_t)strlen(r1);
    s->OnRecvSelf((char*)r1, l);

    //방 생성 이벤트 브로드 캐스팅
    json_object *notice = json_object_new_object();
    json_object_object_add(notice, "cmd", json_object_new_string("created_room"));
    json_object_object_add(notice, "title", json_object_new_string(stitle));
    json_object_object_add(notice, "owned", json_object_new_string(s->getUserid().c_str()));
    json_object_object_add(notice, "user_count", json_object_new_int(1));
    json_object_object_add(notice, "max_count", json_object_new_int(8));

    const char *n = json_object_to_json_string(notice);
    l = (size_t)strlen(n);

    s->OnRecv((char*)n,l);
  
    json_object_put(notice);
    json_object_put(obj);
   
    //json_object_put(notice);

  } else if(!strcmp(scmd, "join_room")) {
    json_object *title = json_object_object_get(obj, "title");
    json_object *user = json_object_object_get(obj, "userid");

    const char *stitle = json_object_get_string(title);
    const char *suser = json_object_get_string(user);
    lwsl_user("title:%s\n", stitle);
    lwsl_user("user:%s\n", suser);
    auto r = lobby_->find(stitle);
    if(r) {
      s->Join(r);
      result = 200;
    } else {
       result = 400;
    }

    json_object_object_add(obj, "result", json_object_new_int(result));
    const char *r1 = json_object_to_json_string(obj);
    size_t l = (size_t)strlen(r1);
    s->OnRecvSelf((char*)r1, l);

  
    std::string i = r->info2();
    lwsl_user("iii:%s\n", i.c_str());

    s->OnRecvRoom((char*)i.c_str(), i.size());
   
    std::string room_info = lobby_->print1();
    
    l = room_info.size();
    s->OnRecvSelf((char*)room_info.c_str(), l);
    s->OnRecv((char*)room_info.c_str(),l); 
  
    /*
      //auto r = lobby_->JoinRoom(obj);
      //if(r) {
      // s->join(r);
      //  json_object_object_add(obj, "result", json_object_new_int(200));
      //   const char* res = json_object_to_json_string(obj);
      //   size_t len = (size_t)strlen(res);
      //   s->OnResponse(res, len);
      //   const char* room_info = r->get_room_info();
      //   s->OnNotifyRoom(room_info, (size_t)strlen(room_info));)
           const char* rs = lobby_->get_rooms();
           if(rs) {
            s->OnNotifyAllSessions(rs, strlen(rs));
           }
      // } else {
      //   json_object_object_add(obj, "result", json_object_new_int(400));
      //   const char* res = json_object_to_json_string(obj);
      //   size_t len = (size_t)strlen(res);
      //   s->OnResponse(res, len);
      //}
    */

    json_object_put(obj);
  } else if(!strcmp(scmd, "chat")) {
    
     const char *n = json_object_to_json_string(obj);
  
    s->SendRoom(std::string(n));
  }

  //json_object_put(res);
  return 1;
}

void WebServer::Send(uint32_t fd, char* _in, size_t len)
{
  for ( auto s : sessions_) {
    if(s.second->get_fd() != fd)
        s.second->Send(_in, len);
  }
}

void WebServer::Send(char* _in, size_t len)
{
   for ( auto s : sessions_) {
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
  lwsl_user("service start\n");
  int n = 0;
  while (n >= 0)
  {
    n = lws_service(context_, 0);
  }
  lwsl_user("service finished\n");
  return 0;
}
