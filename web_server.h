#ifndef _WEB_SERVER_H__
#define _WEB_SERVER_H__

#include <map>
#include <string>
#include <memory>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <libwebsockets.h>

class Session;
class Lobby;
class WebServer
{
public:
  explicit WebServer(const size_t rx_buffer);
  ~WebServer();

  int Initialize(const uint32_t port);

  void Register(uint32_t fd, std::shared_ptr<Session> session);

  void UnRegister(uint32_t fd);

  void OnRecv(uint32_t fd, char *in, size_t len);

  void OnWrite(uint32_t fd);

  void Send(uint32_t fd, char *_in, size_t len);
  void Send(char *_in, size_t len);

  int PreTranslate(std::shared_ptr<Session> ws, char *_in, size_t len);

  int Run();

private:
  const size_t rx_buffer_;
  uint32_t port_;
  struct lws_context_creation_info info_;
  struct lws_context *context_;

  std::shared_ptr<Lobby> lobby_;
  std::map<uint32_t, std::shared_ptr<Session>> sessions_;

  mongoc_client_t *client_;
  mongoc_collection_t *collection_;
};

#endif
