#ifndef SERVER_HPP
# define SERVER_HPP

# include <vector>
# include <map>
# include "config/Config.hpp"
# include "server/ListeningSocket.hpp"
# include "server/Client.hpp"
# include "errno.h"


// The heart of webserv. Owns the listening sockets, the connected clients
// and runs the single poll() loop that drives every I/O operation.

// NOTE: run() is currently a no-op stub. The actual poll loop will
// be implemented later.

class Server {
public:
    explicit Server(const Config& config);
    ~Server();

    void run();
    void stop();

private:
    Server();
    Server(const Server& other);
    Server& operator=(const Server& other);

    // Lifecycle helpers (later)
    void setupListeningSockets();
    void teardown();

    // Poll loop core (later)
    void pollLoop();
    void acceptNewConnection(ListeningSocket* socket);
    void handleClientRead(Client* client);
    void handleClientWrite(Client* client);
    void closeClient(Client* client);
    void checkTimeouts();

    Config                          _config;
    std::vector<ListeningSocket*>   _listeningSockets;
    std::map<int, Client*>          _clients;        // fd -> Client*
    bool                            _running;
};

#endif
