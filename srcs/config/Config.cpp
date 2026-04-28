#include "config/Config.hpp"

Config::Config() : _servers() {}

Config::Config(const Config& other) : _servers(other._servers) {}

Config& Config::operator=(const Config& other) {
    if (this != &other)
        _servers = other._servers;
    return *this;
}

Config::~Config() {}

const std::vector<ServerConfig>& Config::getServers() const { return _servers; }
void Config::addServer(const ServerConfig& server) { _servers.push_back(server); }
bool Config::empty() const { return _servers.empty(); }
size_t Config::size() const { return _servers.size(); }

