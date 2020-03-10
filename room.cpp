



#include "room.h"
#include "web_session.h"
#include "lobby.h"
#include <functional>

void Room::join(SessionPtr user) {
	users.push_back(user);
     json_object *notice = json_object_new_object();
    json_object_object_add(notice, "cmd", json_object_new_string("chat"));
    json_object_object_add(notice, "userid", json_object_new_string(user->getUserid().c_str()));
    json_object_object_add(notice, "content", json_object_new_string("참여하였습니다."));

    const char *n = json_object_to_json_string(notice);
    size_t l = (size_t)strlen(n);

    notify(n);

    json_object_put(notice);

	//notify("[" + std::to_string(number_) + "]" + user->getUserid() + "가 참여하였습니다.");
	//notify("[" + std::to_string(number_) + "] (" + std::to_string(users.size()) + ")");
}

bool remover1( std::string pred,  std::string pred1) {
	return pred == pred1;
}

void Room::leave(SessionPtr user) {

	users.remove(user);
  lwsl_user("leave size111:%d\n", users.size());
  lwsl_user("leave:%s\n", user->getUserid().c_str());

     json_object *notice = json_object_new_object();
    json_object_object_add(notice, "cmd", json_object_new_string("chat"));
    json_object_object_add(notice, "userid", json_object_new_string(user->getUserid().c_str()));
    json_object_object_add(notice, "content", json_object_new_string("나갔습니다."));

    const char *n = json_object_to_json_string(notice);
    size_t l = (size_t)strlen(n);

    notify(n);

    json_object_put(notice);

    auto uu = info2();

    notify(uu);

    
    

//	lwsl_user("%s\n", std::to_string(number_)+"]"+user->getUserid() + "가 나갔습니다.");
	//lwsl_user("%s\n", std::to_string(number_) + "] ("+std::to_string(users.size())+")");
	if (users.size() < 1) {
		auto l = lobby_.lock();
		if (l) {
			l->remove(this->getPtr());
		}
  
	} else {
    auto l = lobby_.lock();
    if(l) {
      l->update();
    }
  }
 
}

void Room::notify(std::string msg) {
	for (auto u : users) {
      u->Send(msg.c_str(), msg.size());
	}
}

void Room::send(std::string from, std::string msg) {
  lwsl_user("send size:%d\n", users.size());
	for (auto u : users) {
      u->Send(msg.c_str(), msg.size());
	}
}

std::string Room::info2() {
    json_object *obj = json_object_new_object();
    json_object_object_add(obj, "cmd", json_object_new_string("room_info"));
    json_object_object_add(obj, "number", json_object_new_int(number_));
    json_object_object_add(obj, "title", json_object_new_string(title_.c_str()));

    json_object *arr = json_object_new_array();
    for (auto u : users) {
      json_object *userid = json_object_new_object();
      json_object_object_add(userid, "userid", json_object_new_string(u->getUserid().c_str()));
      json_object_array_add(arr, userid);
    }

    json_object_object_add(obj, "users", arr);
    const char *sobj = json_object_to_json_string(obj);
    lwsl_user("info2:%s\n", sobj);
    std::string robj(sobj);
    json_object_put(obj);
    return robj;
  }