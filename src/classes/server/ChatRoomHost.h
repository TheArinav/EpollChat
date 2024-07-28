#ifndef EPOLLCHAT_CHATROOMHOST_H
#define EPOLLCHAT_CHATROOMHOST_H

#include <memory>
#include <tuple>

#include "ClientInfo.h"

namespace src::classes::server {

    class ChatRoomHost {
    public:
        unsigned long long RoomID;
        string DisplayName;
        unique_ptr<ClientInfo> Host;
        vector<unique_ptr<ClientInfo>> Members;
        vector<tuple<unsigned long long, string>> Messages;

        explicit ChatRoomHost(string name, unique_ptr<ClientInfo> host);
    private:
        static unsigned long long count;
    };

} // server

#endif //EPOLLCHAT_CHATROOMHOST_H
