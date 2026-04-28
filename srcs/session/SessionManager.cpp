#include "session/SessionManager.hpp"
#include "utils/StringUtils.hpp"
#include <cstdlib>
#include <ctime>
#include <sstream>


// Session
Session::Session() : _id(), _data() {}

Session::Session(const std::string& id) : _id(id), _data() {}

Session::Session(const Session& other) : _id(other._id), _data(other._data) {}

Session& Session::operator=(const Session& other) {
    if (this != &other) {
        _id   = other._id;
        _data = other._data;
    }
    return *this;
}

Session::~Session() {}

const std::string& Session::getId() const { return _id; }

void Session::set(const std::string& key, const std::string& value) {
    _data[key] = value;
}

std::string Session::get(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = _data.find(key);
    if (it == _data.end())
        return "";
    return it->second;
}

bool Session::has(const std::string& key) const {
    return _data.find(key) != _data.end();
}

void Session::remove(const std::string& key) {
    _data.erase(key);
}

// SessionManager (singleton)
SessionManager::SessionManager() : _sessions() {}
SessionManager::~SessionManager() {}

SessionManager& SessionManager::instance() {
    static SessionManager singleton;
    return singleton;
}

std::string SessionManager::createSession() {
    std::string id = generateId();
    _sessions[id] = Session(id);
    return id;
}

Session* SessionManager::findSession(const std::string& sessionId) {
    std::map<std::string, Session>::iterator it = _sessions.find(sessionId);
    if (it == _sessions.end())
        return NULL;
    return &it->second;
}

bool SessionManager::exists(const std::string& sessionId) const {
    return _sessions.find(sessionId) != _sessions.end();
}

void SessionManager::destroySession(const std::string& sessionId) {
    _sessions.erase(sessionId);
}

// Simple unique-enough id for the bonus. We may want to harden it at the end of the project.
std::string SessionManager::generateId() const {
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(NULL)));
        seeded = true;
    }
    std::ostringstream oss;
    oss << std::time(NULL) << "-" << std::rand() << "-" << std::rand();
    return oss.str();
}
