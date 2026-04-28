#include "server/ListeningSocket.hpp"
#include "utils/Logger.hpp"
#include <unistd.h>

ListeningSocket::ListeningSocket()
    : _fd(-1), _host(""), _port(0) {}

ListeningSocket::~ListeningSocket() {
    close();
}

// Stub
// Will perform: socket() -> setsockopt(SO_REUSEADDR) -> bind() ->
//               listen() -> fcntl(O_NONBLOCK).
bool ListeningSocket::open(const std::string& host, int port, int backlog) {
	// Logger::warn("ListeningSocket::open is a stub (no real socket created).");
	int optval = 1;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        Logger::error("Failed to create socket");
        return false;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0){
        Logger::error("Failed to setsockopt");
        ::close(sockfd);
        return false;
    }
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        Logger::error("Invalid address: " + host);
        ::close(sockfd);
        return false;
    }
    int res = bind(sockfd, (const sockaddr*) &addr, sizeof(sockaddr_in));
    if (res < 0){
        Logger::error("Failed to bind");
        ::close(sockfd);
        return false;}
    res = listen(sockfd, backlog);
    if (res < 0){
        Logger::error("Failed to listen");
        ::close(sockfd);
        return false;}
    _fd = sockfd;
    _host = host;
    _port = port;
    return true;
}

void ListeningSocket::close() {
    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1;
    }
}

int                 ListeningSocket::getFd() const   { return _fd; }
const std::string&  ListeningSocket::getHost() const { return _host; }
int                 ListeningSocket::getPort() const { return _port; }
bool                ListeningSocket::isOpen() const  { return _fd >= 0; }
