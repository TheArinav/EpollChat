#ifndef EPOLLCHAT_SERVER_H
#define EPOLLCHAT_SERVER_H

#include <string>
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <thread>
#include <netdb.h>
#include <sys/epoll.h>
#include "ClientInfo.h"
#include "ChatRoomHost.h"
#include "../general/ClientAction.h"
#include "../general/ServerAction.h"

#define SERVER_PORT "3490"
#define MAX_EVENTS 1024

using namespace std;
using namespace src::classes::general;

namespace src::classes::server {

    class Server {
    public:
        string DisplayName;
        int ServerFD;
        int EpollFD;
        atomic<bool> StopFlag;
        thread *ServerThread;
        vector<unique_ptr<ClientInfo>> Clients;
        vector<unique_ptr<ChatRoomHost>> Rooms;
        queue<tuple<unique_ptr<ClientInfo>, ClientAction>> Responses;
        queue<tuple<unique_ptr<ClientInfo>, ServerAction>> Requests;

        explicit Server(string name);
        ~Server();

        void Start();

        void Stop();

    private:
        void Setup();
        void ProcessResponses();
        void EnactRespond();
    };

} // namespace src::classes::server

#endif //EPOLLCHAT_SERVER_H
