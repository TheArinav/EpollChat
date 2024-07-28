//
// Created by ariel on 7/25/2024.
//

#ifndef EPOLLCHAT_CLIENTINFO_H
#define EPOLLCHAT_CLIENTINFO_H

#include <string>
#include <vector>
#include <netinet/in.h>

using namespace std;

namespace src::classes::server {

    class ClientInfo {
    public:
        unsigned long long ClientID;
        string DisplayName;
        //region ConnectionData
        int FileDescriptor;
        sockaddr_storage Address;
        vector<char> ReadBuffer;
        vector<char> WriteBuffer;
        //endregion
        explicit ClientInfo(int fd, sockaddr_storage addr, string  DisplayName);
        ~ClientInfo();

        ssize_t Read();
        ssize_t Write();
        void EnqueueResponse(const string& s_resp);
    private:
        static unsigned long long count;
    };

} // server

#endif //EPOLLCHAT_CLIENTINFO_H
