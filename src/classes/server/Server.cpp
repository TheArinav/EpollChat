#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <tuple>

#include "Server.h"

typedef addrinfo AddressInfo;
typedef epoll_event EpollEvent;

namespace src::classes::server {

    Server::Server(string name) : DisplayName(move(name)), ServerThread(nullptr) {
        Setup();
    }

    void Server::Start() {
        ServerThread = new thread([this]() {
            vector<EpollEvent> events(MAX_EVENTS);
            while (!StopFlag.load()) {
                int n = epoll_wait(EpollFD, events.data(), MAX_EVENTS, -1);
                if (n == -1) {
                    perror("epoll_wait");
                    exit(EXIT_FAILURE);
                }

                for (int i = 0; i < n; ++i) {
                    if (events[i].data.fd == ServerFD) {
                        // Handle new connection
                        sockaddr_storage addr{};
                        socklen_t addr_len = sizeof(addr);
                        int new_fd = accept(ServerFD, (sockaddr*)&addr, &addr_len);
                        if (new_fd == -1) {
                            perror("accept");
                            continue;
                        }

                        // Set non-blocking
                        int flags = fcntl(new_fd, F_GETFL, 0);
                        if (flags == -1) {
                            perror("fcntl");
                            close(new_fd);
                            continue;
                        }
                        flags |= O_NONBLOCK;
                        if (fcntl(new_fd, F_SETFL, flags) == -1) {
                            perror("fcntl");
                            close(new_fd);
                            continue;
                        }

                        // Add new client to epoll
                        EpollEvent event;
                        event.data.fd = new_fd;
                        event.events = EPOLLIN | EPOLLET;
                        if (epoll_ctl(EpollFD, EPOLL_CTL_ADD, new_fd, &event) == -1) {
                            perror("epoll_ctl");
                            close(new_fd);
                            continue;
                        }

                        auto client = make_unique<ClientInfo>(new_fd, addr, "Guest");
                        Clients.push_back(move(client));
                    } else {
                        // Handle client data
                        for (auto& client : Clients) {
                            if (client->FileDescriptor == events[i].data.fd) {
                                ssize_t bytes_read = client->Read();
                                if (bytes_read <= 0) {
                                    if (bytes_read == 0) {
                                        // Connection closed
                                        close(client->FileDescriptor);
                                    } else {
                                        string buff = string(client->ReadBuffer.begin(), client->ReadBuffer.end());
                                        auto curReq = ServerAction::Deserialize(buff);
                                        Requests.push(make_tuple<unique_ptr<ClientInfo>,ServerAction>(make_unique<ClientInfo>(*client), (ServerAction&&)curReq));
                                    }
                                    continue;
                                }
                            }
                        }
                    }
                }
                ProcessResponses();
                EnactRespond();
            }
        });
    }

    void Server::ProcessResponses() {
        size_t itCount = Responses.size();
        for (int i = 0; i < itCount; ++i) {
            auto resp = move(Responses.front());
            Responses.pop();

            auto& [curCl, curAct] = resp;
            curCl->EnqueueResponse(curAct.Serialize());
            curCl->Write();
        }
    }

    void Server::Stop() {
        StopFlag.store(true);
        if (ServerThread && ServerThread->joinable()) {
            ServerThread->join();
        }
    }

    void Server::Setup() {
        StopFlag = false;
        ServerFD = -1;
        EpollFD = epoll_create1(0);
        if (EpollFD == -1) {
            cerr << "Error in epoll_create1:\n\t" << strerror(errno) << endl;
            exit(EXIT_FAILURE);
        }

        AddressInfo hints{}, *server_inf, *p;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        int rv = getaddrinfo(nullptr, SERVER_PORT, &hints, &server_inf);
        if (rv) {
            cerr << "Error in getaddrinfo():\n\t" << gai_strerror(rv) << endl;
            exit(EXIT_FAILURE);
        }

        int p_errno = -1;
        for (p = server_inf; p != nullptr; p = p->ai_next) {
            ServerFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (ServerFD == -1) {
                perror("socket()");
                p_errno = errno;
                continue;
            }
            int yes = 1;
            if (setsockopt(ServerFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
                cerr << "Error in setsockopt():\n\t" << strerror(errno) << endl;
                exit(EXIT_FAILURE);
            }
            if (bind(ServerFD, p->ai_addr, p->ai_addrlen) == -1) {
                perror("bind()");
                p_errno = errno;
                continue;
            }
            break;
        }
        freeaddrinfo(server_inf);

        if (!p) {
            cerr << "Bind failure, cause:\n\t" << strerror(p_errno) << endl;
            exit(EXIT_FAILURE);
        }

        if (listen(ServerFD, 10) == -1) {
            cerr << "Listen failure, cause:\n\t" << strerror(errno) << endl;
            exit(EXIT_FAILURE);
        }

        int flags = fcntl(ServerFD, F_GETFL, 0);
        if (flags == -1) {
            cerr << "Error in fcntl:\n\t" << strerror(errno) << endl;
            exit(EXIT_FAILURE);
        }
        flags |= O_NONBLOCK;
        if (fcntl(ServerFD, F_SETFL, flags) == -1) {
            cerr << "Error in fcntl:\n\t" << strerror(errno) << endl;
            exit(EXIT_FAILURE);
        }

        EpollEvent event{};
        event.data.fd = ServerFD;
        event.events = EPOLLIN;
        if (epoll_ctl(EpollFD, EPOLL_CTL_ADD, ServerFD, &event) == -1) {
            cerr << "Error in epoll_ctl:\n\t" << strerror(errno) << endl;
            exit(EXIT_FAILURE);
        }
    }

    Server::~Server() {
        if (fcntl(ServerFD, F_GETFD) != -1 || errno != EBADFD) {
            close(ServerFD);
        }
        StopFlag.store(true);
        if (ServerThread && ServerThread->joinable()) {
            ServerThread->join();
        }
        delete ServerThread;
        Clients.clear();
        Rooms.clear();
        while (!Responses.empty()) {
            Responses.pop();
        }
        while (!Requests.empty()) {
            Requests.pop();
        }
    }

    void Server::EnactRespond() {

    }

} // namespace src::classes::server
