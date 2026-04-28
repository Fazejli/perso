#ifndef LISTENINGSOCKET_HPP
# define LISTENINGSOCKET_HPP

# include <string>

#include "Webserv.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>

// Wraps a non-blocking TCP socket bound to a `host:port` and put in
// listen() mode. Owns its file descriptor — non-copyable.

class ListeningSocket {
	public:
   		ListeningSocket();
    	~ListeningSocket();

    	bool open(const std::string& host, int port, int backlog);
    	void close();

    	int                getFd() const;
    	const std::string& getHost() const;
    	int                getPort() const;
    	bool               isOpen() const;

	private:
    	ListeningSocket(const ListeningSocket& other);
    	ListeningSocket& operator=(const ListeningSocket& other);

    	int          _fd;
    	std::string  _host;
    	int          _port;
};

#endif
