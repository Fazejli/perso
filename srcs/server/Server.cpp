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
    Logger::info("Server::run() —  stub.");
    Logger::info("  parsed servers in config: " + StringUtils::toString(_config.size()));
    Logger::info("  We will set up listening sockets and start the poll loop later.");
    Logger::info("Nothing to do for now, exiting cleanly.");
}

void Server::stop() {
    _running = false;
}

// Stub
void Server::setupListeningSockets() {
    // iterate over _config.getServers(), build one ListeningSocket
    // per unique host:port and store them in _listeningSockets.
    ListeningSocket* sock1 = new ListeningSocket();
    if (!sock1->open(_config.getServers()[0].getHost(), _config.getServers()[0].getPort(), DEFAULT_BACKLOG)) {
        Logger::error("Failed to set up listening socket for " + _config.getServers()[0].getHost() + ":" + StringUtils::toString(_config.getServers()[0].getPort()));
        delete sock1;
        return;
    }
    _listeningSockets.push_back(sock1);
    
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
        // Build pollfd array from _listeningSockets and _clients
        // Call poll()
        // For each returned event:
        //   - if it's a listening socket, call acceptNewConnection()
        //   - if it's a client socket, call handleClientRead() or
        //     handleClientWrite() depending on the event type.
        // Periodically call checkTimeouts() to close timed-out clients.

    }
}

void Server::acceptNewConnection(ListeningSocket* /*s*/)   { /* later */ }
void Server::handleClientRead(Client* /*client*/)          { /* later */ }
void Server::handleClientWrite(Client* /*client*/)         { /* later */ }
void Server::closeClient(Client* /*client*/)               { /* later */ }
void Server::checkTimeouts()                               { /* later */ }
