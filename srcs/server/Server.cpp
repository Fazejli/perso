#include "server/Server.hpp"
#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"

Server::Server(const Config& config)
    : _config(config),
      _listeningSockets(),
      _clients(),
      _running(false) {
}

Server::~Server() {
    teardown();
}

void Server::run() {
    Logger::info("Server::run()");
    Logger::info("  parsed servers in config: " + StringUtils::toString(_config.size()));
    Logger::info("  We are setting up listening sockets");
    setupListeningSockets();
    Logger::info("Polling up");
    pollLoop();
    Logger::info("Nothing to do for now, exiting cleanly.");
    
}

void Server::stop() {
    _running = false;
}

bool checkDuplicate(std::vector<ListeningSocket*> sockets, const std::string host, int port){
    std::vector<ListeningSocket*>::const_iterator it = sockets.begin();
    while (it != sockets.end()){
        if ((*it)->getPort() == port && (*it)->getHost() == host)
            return true;
        ++it;
    }
    return false;
}

void Server::setupListeningSockets() {
    // iterate over _config.getServers(), build one ListeningSocket
    // per unique host:port and store them in _listeningSockets.
    std::vector<ServerConfig> servers = _config.getServers();
    std::vector<ServerConfig>::iterator it = servers.begin();
    while (it != servers.end())
    {
        const std::string host = it->getHost();
        int port = it->getPort();
        if (checkDuplicate(_listeningSockets, host, port))
            continue ;
        ListeningSocket *s = new ListeningSocket;
        if (!s->open(host, port, DEFAULT_BACKLOG)){
            Logger::error("Failed to open socket..");
            delete s;
            continue ;
        }
        _listeningSockets.push_back(s);
        Logger::info("Listenning on " + host + ":" + StringUtils::toString(port));
    }
    _running = true;
}

void Server::teardown() {
    // Free clients
    for (std::map<int, Client*>::iterator it = _clients.begin();
         it != _clients.end(); ++it) {
        delete it->second;
    }
    _clients.clear();

    // Free listening sockets
    for (std::vector<ListeningSocket*>::iterator it = _listeningSockets.begin();
         it != _listeningSockets.end(); ++it) {
        delete *it;
    }
    _listeningSockets.clear();
}

void Server::pollLoop(){
    while (_running) {
        std::vector<struct pollfd> fds;
        std::vector<ListeningSocket*>::const_iterator it = _listeningSockets.begin();
        while (it != _listeningSockets.end()){
            struct pollfd pfd;
            pfd.fd = (*it)->getFd();
            pfd.events = POLLIN;
            pfd.revents = 0;
            fds.push_back(pfd);
            ++it;
        }
        std::map<int, Client*>::const_iterator iter = _clients.begin();
        while (iter != _clients.end()){
            struct pollfd pfd;
            Client* client = iter->second;
            pfd.fd = client->getFd();
            pfd.events = 0;
            pfd.revents = 0;
            if (client->getState() == Client::READING)
                pfd.events = POLLIN;
            else if (client->getState() == Client::WRITING)
                pfd.events = POLLOUT;
            fds.push_back(pfd);
            ++iter;
        }
        if (poll(fds.data(), fds.size(), CLIENT_TIMEOUT_SECONDS) < 0){
            Logger::error("Failed to poll..");
            break ;
        }
        size_t count = _listeningSockets.size();
        for(size_t i = 0; i < count; i++){
            if (fds[i].revents == 0)
                continue;
            if (i < count){
                if (fds[i].revents & POLLIN)
                    acceptNewConnection(_listeningSockets[i]);
                else{
                    int fd = fds[i].fd;
                    Client* client = _clients[fd];
                    if (fds[i].revents & (POLLERR | POLLHUP))
                        closeClient(client);
                    else if (fds[i].revents & POLLIN)
                        handleClientRead(client);
                    else if (fds[i].revents & POLLOUT)
                        handleClientWrite(client); 
                }
            }
        }
        //checkTimeouts();
    }
}

void Server::acceptNewConnection(ListeningSocket* sock){
    struct sockaddr addr;
    socklen_t addrLen = sizeof(addr);
    int clientfd = accept(sock->getFd(), &addr, &addrLen);
    if (clientfd < 0){
        Logger::error("Failed to accept new connection..");
        return ;
    }
    Logger::info("New connection accepted.");
    Client* client = new Client(clientfd);
    _clients[clientfd] = client;
}

void Server::handleClientRead(Client* c){
    char buffer[DEFAULT_BUFFER_SIZE];
    ssize_t msg_received = recv(c->getFd(), buffer, sizeof(buffer), 0);
    if (msg_received < 0){
        if (errno != EWOULDBLOCK && errno != EAGAIN){
            Logger::error("Failed to read from client..");
            closeClient(c);
        }
        Logger::warn("No data to read from client.");
        return ;
    }
    else if (!msg_received){
        Logger::info("Client closed the connection.");
        closeClient(c);
        return ;
    }
    c->getReadBuffer().append(buffer, msg_received);
    c->touch();
    Logger::info("Received data from client");
}

void Server::handleClientWrite(Client* c){
    const std::string& data = c->getWriteBuffer();
    ssize_t msg_sent = send(c->getFd(), data.c_str(), data.size(), 0);
    if (msg_sent < 0){
        if (errno != EWOULDBLOCK && errno != EAGAIN){
            Logger::error("Failed to write to client..");
            closeClient(c);
        }
        Logger::warn("No data can be sent to client right now.");
        return ;
    }
    c->getWriteBuffer().erase(0, msg_sent);
    c->touch();
    Logger::info("Sent data to client");
}

void Server::closeClient(Client* c){
    int fd = c->getFd();
    delete c;
    _clients.erase(fd);
    Logger::info("Closed client connection.");
}

void Server::checkTimeouts(){
    time_t now = std::time(NULL);
    std::vector<int> timedOutClients;
    for (std::map<int, Client*>::const_iterator it = _clients.begin();
         it != _clients.end(); ++it) {
        Client* client = it->second;
        if (client->hasTimedOut(now, CLIENT_TIMEOUT_SECONDS)) {
            timedOutClients.push_back(it->first);
        }
    }
    for (size_t i = 0; i < timedOutClients.size(); ++i) {
        int fd = timedOutClients[i];
        Logger::info("Client " + StringUtils::toString(fd) + " has timed out.");
        closeClient(_clients[fd]);
    }
}
