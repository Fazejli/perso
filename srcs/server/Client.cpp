#include "server/Client.hpp"
#include <unistd.h>

Client::Client(int fd)
    : _fd(fd),
      _state(READING),
      _request(),
      _response(),
      _readBuffer(),
      _writeBuffer(),
      _serverConfig(NULL),
      _lastActivity(std::time(NULL)) {
}

Client::~Client() {
    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1;
    }
}

int            Client::getFd() const         { return _fd; }
Client::State  Client::getState() const      { return _state; }
void           Client::setState(State state) { _state = state; }

HttpRequest&        Client::getRequest()        { return _request; }
const HttpRequest&  Client::getRequest() const  { return _request; }
HttpResponse&       Client::getResponse()       { return _response; }
const HttpResponse& Client::getResponse() const { return _response; }

std::string&  Client::getReadBuffer()  { return _readBuffer; }
std::string&  Client::getWriteBuffer() { return _writeBuffer; }

void                Client::setServerConfig(const ServerConfig* config) { _serverConfig = config; }
const ServerConfig* Client::getServerConfig() const                     { return _serverConfig; }

time_t Client::getLastActivity() const { return _lastActivity; }
void   Client::touch()                 { _lastActivity = std::time(NULL); }

bool Client::hasTimedOut(time_t now, int timeoutSeconds) const {
    return (now - _lastActivity) > timeoutSeconds;
}
