#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <ctime>
# include "http/HttpRequest.hpp"
# include "http/HttpResponse.hpp"

class ServerConfig;


// Represents one accepted TCP connection. Holds the in-flight request being
// parsed, the response being built, and the I/O buffers. Owns its socket
// fd — non-copyable. The Server stores `Client*` in a map keyed by fd.

// State machine:
//   READING    -> we are still parsing headers / body from the client
//   PROCESSING -> the request is complete, we're building the response
//                 (including waiting on a CGI sub-process)
//   WRITING    -> we have data in _writeBuffer to flush to the socket
//   CLOSING    -> response fully sent, the Server should close us

class Client {
	public:
    	enum State {
    	    READING,
        	PROCESSING,
        	WRITING,
        	CLOSING
    	};

    	explicit Client(int fd);
    	~Client();

    	int             getFd() const;
    	State           getState() const;
    	void            setState(State state);

    	HttpRequest&        getRequest();
    	const HttpRequest&  getRequest() const;
    	HttpResponse&       getResponse();
    	const HttpResponse& getResponse() const;

    	std::string&        getReadBuffer();
    	std::string&        getWriteBuffer();

    	// Pointer to the ServerConfig that should handle this client (chosen
    	// when the request line is parsed and we know which listening socket
    	// accepted it / which Host header was sent).
    	void                       setServerConfig(const ServerConfig* config);
    	const ServerConfig*        getServerConfig() const;

    	time_t          getLastActivity() const;
    	void            touch();   // refresh _lastActivity to "now"
    	bool            hasTimedOut(time_t now, int timeoutSeconds) const;

	private:
    	Client();
    	Client(const Client& other);
    	Client& operator=(const Client& other);

    	int                  _fd;
    	State                _state;
    	HttpRequest          _request;
    	HttpResponse         _response;
    	std::string          _readBuffer;
    	std::string          _writeBuffer;
    	const ServerConfig*  _serverConfig;
    	time_t               _lastActivity;
};

#endif
