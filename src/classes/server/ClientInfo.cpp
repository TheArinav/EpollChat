//
// Created by ariel on 7/25/2024.
//

#include "ClientInfo.h"
#include "unistd.h"
#include "iostream"

#include <utility>
#include <fcntl.h>
#include <cstring>

namespace src::classes::server {
    unsigned long long ClientInfo::count = 0;

    ClientInfo::ClientInfo(int fd, sockaddr_storage addr, string displayName) :
            ClientID(count++),
            DisplayName(move(displayName)),
            FileDescriptor(fd),
            Address(addr) {
        ReadBuffer = {};
        WriteBuffer = {};
    }

    ssize_t ClientInfo::Read() {
        char buffer[1024] = {0};
        ssize_t res = recv(FileDescriptor, buffer, sizeof(buffer), 0);
        if (res > 0) {
            ReadBuffer.insert(ReadBuffer.end(), buffer, buffer + res);
        } else if (res == 0) {
            // Handle client disconnection if needed
        } else {
            // Handle errors (EAGAIN and EWOULDBLOCK are not errors in non-blocking mode)
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv");
                cerr << "Error in recv():\n\t" << strerror(errno) << "\n";
            }
        }
        return res;
    }

    ssize_t ClientInfo::Write() {
        if (WriteBuffer.empty())
            return 0;
        ssize_t res = send(FileDescriptor, WriteBuffer.data(), WriteBuffer.size(), 0);
        if (res > 0) {
            WriteBuffer.erase(WriteBuffer.begin(), WriteBuffer.begin() + res);
        } else if (res == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("send()");
            cerr << "Error in send():\n\t" << strerror(errno) << "\n";
        }
        return res;
    }

    void ClientInfo::EnqueueResponse(const string &s_resp) {
        WriteBuffer.insert(WriteBuffer.end(), s_resp.begin(), s_resp.end());
    }

    ClientInfo::~ClientInfo() {
        if (fcntl(FileDescriptor, F_GETFD) != -1 || errno != EBADF) {
            close(FileDescriptor);
        }
        WriteBuffer.clear();
        ReadBuffer.clear();
        DisplayName.clear();
    }
} // server