#include <iostream>
#include "web_server.h"

int main()
{

  WebServer server(4096);

  server.Initialize(5600);

  return server.Run();
}
