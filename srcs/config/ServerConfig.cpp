#include "config/ServerConfig.hpp"
#include "Webserv.hpp"

ServerConfig::ServerConfig()
    : _host(DEFAULT_HOST),
      _port(DEFAULT_PORT),
      _serverNames(),
      _clientMaxBodySize(DEFAULT_CLIENT_MAX_BODY_SIZE),
      _errorPages(),
      _locations() {
}

ServerConfig::ServerConfig(const ServerConfig& other)
    : _host(other._host),
      _port(other._port),
      _serverNames(other._serverNames),
      _clientMaxBodySize(other._clientMaxBodySize),
      _errorPages(other._errorPages),
      _locations(other._locations) {
}

ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) {
        _host              = other._host;
        _port              = other._port;
        _serverNames       = other._serverNames;
        _clientMaxBodySize = other._clientMaxBodySize;
        _errorPages        = other._errorPages;
        _locations         = other._locations;
    }
    return *this;
}

ServerConfig::~ServerConfig() {}

const std::string&                  ServerConfig::getHost() const               { return _host; }
int                                 ServerConfig::getPort() const               { return _port; }
const std::vector<std::string>&     ServerConfig::getServerNames() const        { return _serverNames; }
size_t                              ServerConfig::getClientMaxBodySize() const  { return _clientMaxBodySize; }
const std::map<int, std::string>&   ServerConfig::getErrorPages() const         { return _errorPages; }
const std::vector<LocationConfig>&  ServerConfig::getLocations() const          { return _locations; }

void ServerConfig::setHost(const std::string& host)              { _host = host; }
void ServerConfig::setPort(int port)                             { _port = port; }
void ServerConfig::addServerName(const std::string& name)        { _serverNames.push_back(name); }
void ServerConfig::setClientMaxBodySize(size_t size)             { _clientMaxBodySize = size; }
void ServerConfig::addErrorPage(int code, const std::string& path) { _errorPages[code] = path; }
void ServerConfig::addLocation(const LocationConfig& location)   { _locations.push_back(location); }
void ServerConfig::setLocations(const std::vector<LocationConfig>& locations) { _locations = locations; }

const LocationConfig* ServerConfig::findLocation(const std::string& uri) const {
    const LocationConfig* best       = NULL;
    std::string::size_type bestLen   = 0;

    for (std::vector<LocationConfig>::const_iterator it = _locations.begin();
         it != _locations.end(); ++it) {
        const std::string& path = it->getPath();
        if (uri.compare(0, path.size(), path) == 0) {
            if (best == NULL || path.size() > bestLen) {
                best    = &(*it);
                bestLen = path.size();
            }
        }
    }
    return best;
}

std::string ServerConfig::getErrorPagePath(int code) const {
    std::map<int, std::string>::const_iterator it = _errorPages.find(code);
    if (it == _errorPages.end())
        return "";
    return it->second;
}
