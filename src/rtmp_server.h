#pragma once

#include "config.h"
#include "rtmp_define.h"

#include <memory>
#include <unordered_map>

class RtmpServer {
public:
    RtmpServer(int port = RTMP_PORT);
    ~RtmpServer();

    void Start();
    void Stop();

private:
    void DoEpollHandler();
    void SendData(Client* client);
    void RecvData(Client* client);
    void HandleMessage(Client*, RtmpMessage* msg);
    void DoHandShark(Client* client);

    size_t SendAll(int fd, const void* buf, size_t len);
    size_t RecvAll(int fd, void* buf, size_t len);

    int listen_fd_;
    int port_;
    int epollfd_;
    std::unordered_map<int, Client*> clients_;
};