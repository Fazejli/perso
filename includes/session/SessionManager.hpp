#ifndef SESSIONMANAGER_HPP
# define SESSIONMANAGER_HPP

# include <string>
# include <map>

// Session  (BONUS)

// In-memory key/value store associated with a single session id. The
// SessionManager is responsible for creating, fetching and destroying
// these.

class Session {
public:
    Session();
    explicit Session(const std::string& id);
    Session(const Session& other);
    Session& operator=(const Session& other);
    ~Session();

    const std::string& getId() const;

    void                set(const std::string& key, const std::string& value);
    std::string         get(const std::string& key) const;
    bool                has(const std::string& key) const;
    void                remove(const std::string& key);

private:
    std::string                         _id;
    std::map<std::string, std::string>  _data;
};

// SessionManager (BONUS)

// Singleton owning every active session. Sessions are looked up by id, the
// id itself is communicated to the client via a Set-Cookie header.
class SessionManager {
public:
    static SessionManager& instance();

    // Create a brand new session with a unique id, return that id.
    std::string createSession();

    // Lookup. Returns NULL if no session has that id.
    Session*    findSession(const std::string& sessionId);
    bool        exists(const std::string& sessionId) const;
    void        destroySession(const std::string& sessionId);

private:
    SessionManager();
    ~SessionManager();
    SessionManager(const SessionManager& other);
    SessionManager& operator=(const SessionManager& other);

    std::string generateId() const;

    std::map<std::string, Session> _sessions;
};

#endif
