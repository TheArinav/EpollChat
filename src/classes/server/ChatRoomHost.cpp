//
// Created by ariel on 7/25/2024.
//

#include "ChatRoomHost.h"

namespace src::classes::server {
    unsigned long long ChatRoomHost::count = 0;
    ChatRoomHost::ChatRoomHost(string name, unique_ptr<ClientInfo> host) :
    RoomID(count++),DisplayName(move(name)), Host(move(host)){
        Members.push_back(make_unique<ClientInfo>(*Host));
        Messages={};
    }
} // server