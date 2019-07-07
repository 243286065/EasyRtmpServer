#include "rtmp_server.h"
#include "util.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

static const size_t HEADER_LENGTH[] = {12, 8, 4, 1};

int set_nonblock(int fd, bool enabled)
{
	int flags = fcntl(fd, F_GETFL) & ~O_NONBLOCK;
	if (enabled) {
		flags |= O_NONBLOCK;
	}
	return fcntl(fd, F_SETFL, flags);
}

RtmpServer::RtmpServer(int port):port_(port) {}

RtmpServer::~RtmpServer() {}

void RtmpServer::Start() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd_ < 0) {
        std::cout << "Init socket failed! exit..." << std::endl;
        //TODO： 错误处理
        return;
    }

    // 端口复用
    int opt = 1;
    if(setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt)) < 0) {
        std::cout << "setsockopt failed! exit..." << std::endl;
    }

    sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port_);
	sin.sin_addr.s_addr = INADDR_ANY;
    if (bind(listen_fd_, (sockaddr *) &sin, sizeof sin) < 0) {
		throw std::runtime_error(std::string("Unable to listen, error:") + strerror(errno));
		return ;
	}

    // 开始监听请求
    listen(listen_fd_, 10);

    // 处理epool
    DoEpollHandler();
}

void RtmpServer::DoEpollHandler() {
    // 使用epool处理请求
    epoll_event ev, evClients[MAX_EPOLL_CLIENT];
    epollfd_ = epoll_create(MAX_EPOLL_CLIENT);
    if(epollfd_ < 0) {
        throw std::runtime_error(std::string("Unable to epoll_create, error:") + strerror(errno));
        return;
    }

    ev.data.fd = listen_fd_;
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;

    sockaddr_in sin;
    if(epoll_ctl(epollfd_, EPOLL_CTL_ADD, listen_fd_, &ev)<0){
        throw std::runtime_error(std::string("Unable to epoll_ctl, error:") + strerror(errno));
        return;
    }
    
    int num = 0;
    socklen_t length = 0;
    while(true) {
        num = epoll_wait(epollfd_, evClients, MAX_EPOLL_CLIENT, 100);
        //std::cout << "---------" << num  << std::endl;
        for(int i = 0; i < num; ++i) {
            int fd = evClients[i].data.fd;
            if(fd == listen_fd_) {
                //新连接
                int nfd = accept(listen_fd_, (sockaddr *)&sin, &length);
                if(nfd < 0) {
                    std::cerr << " nfd < 0" << std::endl;
                }
                char ip[32];
                std::cout << "accept from: " << inet_ntop(AF_INET, &sin.sin_addr.s_addr, ip, sizeof(sin)) << ":" << sin.sin_port << std::endl;
                epoll_event ev;
                ev.data.fd = nfd;
                ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP ;
                fcntl(nfd, F_SETFL, fcntl(nfd, F_GETFL) | O_NONBLOCK);
                if(epoll_ctl(epollfd_, EPOLL_CTL_ADD, nfd, &ev)<0) {
                    throw std::runtime_error(std::string("Unable to EPOLL_CTL_ADD, error:") + strerror(errno));
                }
                Client* client = new Client;
                client->fd = nfd;
                client->pullStream = false;
                client->needKeyFrame = false;
                client->read_seq = 0;
                client->written_seq = 0;
                client->chunk_len = DEFAULT_CHUNK_LEN;
                for (int i = 0; i < 64; ++i) {
                    client->messages[i].timestamp = 0;
                    client->messages[i].len = 0;
                }
                clients_.insert(std::pair<int, Client*>(nfd, client));
            } else if(evClients[i].events & EPOLLRDHUP){
                // 对端断开
                std::cout << "A client disconnect." << std::endl;
                if(epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &evClients[i])<0) {
                    throw std::runtime_error(std::string("Unable to EPOLL_CTL_DEL, error:") + strerror(errno));
                }
                close(fd);
                if(clients_.find(fd) != clients_.end()) {
                    clients_.erase(fd);
                }
            } else {
                if(fd < 0 || clients_.find(fd) == clients_.end()){
                    continue;
                }
                if(evClients[i].events & EPOLLOUT) {
                    // 可以向对端发送数据
                    std::cout << "A client wait to recevice data." << std::endl;
                    if(clients_.find(fd) != clients_.end())
                        SendData(clients_[fd]);
                }

                if(evClients[i].events & EPOLLIN) {
                    std::cout << "A client send data." << std::endl;
                    // 对端有数据需要处理
                    RecvData(clients_[fd]);
                }
            }
        }
    }

}

void RtmpServer::SendData(Client* client) {
    if(client == nullptr || client->send_queue.size() == 0) {
        return;
    }
    size_t len = client->send_queue.size();
    ssize_t written = send(client->fd, client->send_queue.data(), len, 0);
	if (written < 0) {
		if (errno == EAGAIN || errno == EINTR)
			return;
		throw std::runtime_error(std::string("unable to write to a client: ") + 
						strerror(errno));
	}

	client->send_queue.erase(0, written);
}

void RtmpServer::RecvData(Client* client) {
    if(client == nullptr)
        return;
    std::string chunk(4096, 0);
    ssize_t len = recv(client->fd, &chunk[0], chunk.size(), 0);
    if(len == 0) {
        throw std::runtime_error(std::string("EOF from a client"));
    }else if (len < 0) {
		if (errno == EAGAIN || errno == EINTR)
			return;
		throw std::runtime_error(std::string("unable to read from a client: ") + strerror(errno));
	}
    client->buf.append(chunk, 0, len);
    std::cout << chunk << std::endl;
    while (!client->buf.empty()) {
        uint8_t flags = client->buf[0];
        size_t header_len = HEADER_LENGTH[flags >> 6];

        if (client->buf.size() < header_len) {
			/* need more data */
			break;
		}
        RtmpHeader header;
		memcpy(&header, client->buf.data(), header_len);
        //std::cout << header.header_type << "----" << header.timestamp << "----" << header.amf_type << "---" << header.amf_len << "----" << header.stream_id << std::endl;
        RtmpMessage *msg = &client->messages[flags & 0x3f];

        if (header_len >= 8) {
            // 读取streamID
			msg->len = ReadBigEndian24(header.amf_len);
			if (msg->len < msg->buf.size()) {
				throw std::runtime_error("invalid msg length");
			}
			msg->type = header.amf_type;
		}

        if (header_len >= 12) {
			    msg->endpoint = ReadLittleEndian24(header.stream_id);
		}
        if (msg->len == 0) {
			throw std::runtime_error("message without a header");
		}

        // 数据块的大小
		size_t chunk = msg->len - msg->buf.size();
        if (chunk > client->chunk_len)
			chunk = client->chunk_len;
        
        if (client->buf.size() < header_len + chunk) {
			/* need more data */
			break;
		}

        // 首个包是绝对时间戳，第二个包开始就是相对时间戳
        if (header_len >= 4) {
			unsigned long ts = ReadBigEndian24(header.timestamp);
			if (ts == 0xffffff) {
				throw std::runtime_error("ext timestamp not supported");
			}
			if (header_len < 12) {
				ts += msg->timestamp;
			}
			msg->timestamp = ts;
		}

		msg->buf.append(client->buf, header_len, chunk);
		client->buf.erase(0, header_len + chunk);

		if (msg->buf.size() == msg->len) {
            // 刚好凑齐一个完整的包，对它进行处理
			HandleMessage(client, msg);
			msg->buf.clear();
		}
    }
}

void RtmpServer::HandleMessage(Client*, RtmpMessage* msg) {
    printf("RTMP message %02x, len %zu, timestamp %ld\n", msg->type, msg->len,
		msg->timestamp);
}