#include "config/ConfigParser.hpp"
#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"
#include "Webserv.hpp"

#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdlib>
#include <set>
#include <unistd.h>

ConfigParser::ParseError::ParseError(const std::string& msg)
    : std::runtime_error(msg) {}

ConfigParser::ConfigParser()
    : _tokens(),
      _pos(0),
      _filepath(),
      _currentLocFlags(),
      _currentServerFlags(),
      _serverRoot(),
      _serverIndex(),
      _serverAutoindex(false),
      _serverClientMaxBodySize(DEFAULT_CLIENT_MAX_BODY_SIZE),
      _serverErrorPages(),
      _allLocationFlags() {
}

ConfigParser::~ConfigParser() {}

Config ConfigParser::parse(const std::string& filepath) {
    _filepath = filepath;

    Logger::info("Parsing configuration file: " + filepath);

    std::string source = readFile(filepath);
    _tokens = tokenize(source);
    _pos = 0;

    Logger::debug("Tokenized " + StringUtils::toString(_tokens.size()) + " tokens from source.");

    Config config = buildConfig();
    validateConfig(config);

    Logger::info("Configuration loaded: " + StringUtils::toString(config.size()) + " server block(s).");
    return config;
}

std::string ConfigParser::readFile(const std::string& filepath) const {
    std::ifstream file(filepath.c_str());
    if (!file.is_open())
        throw ParseError("Cannot open configuration file: " + filepath);

    std::ostringstream oss;
    oss << file.rdbuf();
    if (file.bad())
        throw ParseError("Read error on configuration file: " + filepath);
    return oss.str();
}

// Tokenizer
//
// Splits the source into WORD / { / } / ; tokens. Skips '#' line comments
// and arbitrary whitespace. Supports double-quoted words for values
// containing spaces (e.g. error_page paths).
std::vector<ConfigParser::Token> ConfigParser::tokenize(const std::string& source) const {
    std::vector<Token> tokens;
    size_t i = 0;
    size_t line = 1;
    const size_t n = source.size();

    while (i < n) {
        char c = source[i];

        if (c == '\n') { ++line; ++i; continue; }
        if (c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f') {
            ++i; continue;
        }
        if (c == '#') {
            while (i < n && source[i] != '\n') ++i;
            continue;
        }
        if (c == '{') { tokens.push_back(Token(Token::OPEN_BRACE,  "{", line)); ++i; continue; }
        if (c == '}') { tokens.push_back(Token(Token::CLOSE_BRACE, "}", line)); ++i; continue; }
        if (c == ';') { tokens.push_back(Token(Token::SEMICOLON,   ";", line)); ++i; continue; }

        // quoted word
        if (c == '"' || c == '\'') {
            char quote = c;
            size_t startLine = line;
            ++i;
            std::string word;
            bool closed = false;
            while (i < n) {
                if (source[i] == '\n') ++line;
                if (source[i] == quote) { closed = true; ++i; break; }
                word += source[i];
                ++i;
            }
            if (!closed) {
                std::ostringstream oss;
                oss << "Unterminated quoted string starting at line " << startLine << " in " << _filepath;
                throw ParseError(oss.str());
            }
            tokens.push_back(Token(Token::WORD, word, startLine));
            continue;
        }

        // unquoted word
        std::string word;
        size_t startLine = line;
        while (i < n) {
            char d = source[i];
            if (d == ' ' || d == '\t' || d == '\r' || d == '\n' || d == '\v' || d == '\f' || d == '{' || d == '}' || d == ';' || d == '#') {
                break;
            }
            word += d;
            ++i;
        }
        tokens.push_back(Token(Token::WORD, word, startLine));
    }

    tokens.push_back(Token(Token::END, "", line));
    return tokens;
}

// Top-level: walk the token stream, parse one server block at a time
Config ConfigParser::buildConfig() {
    Config config;
    while (current().type != Token::END) {
        if (current().type != Token::WORD || current().value != "server")
            error("expected 'server' block, got '" + current().value + "'", current().line);
        config.addServer(parseServerBlock());
    }
    return config;
}

// server { ... }
ServerConfig ConfigParser::parseServerBlock() {
    advance(); // consume "server"
    expect(Token::OPEN_BRACE, "'{' after 'server'");

    ServerConfig server;
    _currentServerFlags = ServerFlags();
    _serverRoot.clear();
    _serverIndex.clear();
    _serverAutoindex = false;
    _serverClientMaxBodySize = DEFAULT_CLIENT_MAX_BODY_SIZE;
    _serverErrorPages.clear();
    _allLocationFlags.clear();

    while (current().type != Token::CLOSE_BRACE && current().type != Token::END)
        parseServerDirective(server);
    expect(Token::CLOSE_BRACE, "'}' to close server block");

    // Apply server defaults to locations that did not override them.
    std::vector<LocationConfig> locations = server.getLocations();
    for (size_t i = 0; i < locations.size(); ++i) {
        const LocationFlags& flags = _allLocationFlags[i];
        const std::string& locPath = locations[i].getPath();

        if (!flags.hasRoot && _currentServerFlags.hasRoot) {
            locations[i].setRoot(_serverRoot);
            Logger::debug("  Location " + locPath + ": inherited root=" + _serverRoot + " from server");
        }
        if (!flags.hasIndex && _currentServerFlags.hasIndex) {
            locations[i].setIndex(_serverIndex);
            Logger::debug("  Location " + locPath + ": inherited index=" + _serverIndex + " from server");
        }
        if (!flags.hasAutoindex && _currentServerFlags.hasAutoindex) {
            locations[i].setAutoindex(_serverAutoindex);
            Logger::debug("  Location " + locPath + ": inherited autoindex=" + (_serverAutoindex ? "on" : "off") + " from server");
        }
        if (!flags.hasClientMaxBodySize && _currentServerFlags.hasClientMaxBodySize) {
            locations[i].setClientMaxBodySize(_serverClientMaxBodySize);
            Logger::debug("  Location " + locPath + ": inherited client_max_body_size=" + StringUtils::toString(_serverClientMaxBodySize) + " from server");
        }

        // error_page: code-by-code merge so the location can override only
        // some codes and inherit the rest.
        for (std::map<int, std::string>::const_iterator it = _serverErrorPages.begin(); it != _serverErrorPages.end(); ++it) {
            if (locations[i].getErrorPages().find(it->first) == locations[i].getErrorPages().end()) {
                locations[i].addErrorPage(it->first, it->second);
                Logger::debug("  Location " + locPath + ": inherited error_page " + StringUtils::toString(it->first) + " -> " + it->second + " from server");
            }
        }
    }

    // Push the enriched locations back into the server.
    server.setLocations(locations);

    // Log server summary.
    std::string summary = "Server block parsed: " + server.getHost() + ":" + StringUtils::toString(server.getPort());
    if (!server.getServerNames().empty()) {
        summary += " (server_name:";
        for (size_t i = 0; i < server.getServerNames().size(); ++i)
            summary += " " + server.getServerNames()[i];
        summary += ")";
    }
    summary += ", " + StringUtils::toString(locations.size()) + " location(s)";
    Logger::info(summary);

    return server;
}

// One directive inside a server block (or a nested location block)
void ConfigParser::parseServerDirective(ServerConfig& server) {
    if (current().type != Token::WORD)
        error("expected directive name, got '" + current().value + "'", current().line);

    std::string name = current().value;
    size_t      line = current().line;

    if (name == "location") {
        advance(); // consume "location"
        LocationConfig loc = parseLocationBlock();
        server.addLocation(loc);
        _allLocationFlags.push_back(_currentLocFlags);
        return;
    }

    advance(); // consume directive name
    std::vector<Token> args = collectDirectiveArgs();

    if (name == "listen") {
        parseListen(server, args, line);
        _currentServerFlags.hasListen = true;
    }
    else if (name == "host") {
        if (args.size() != 1)
            error("host takes exactly one argument", line);
        server.setHost(args[0].value);
    }
    else if (name == "server_name") {
        if (args.empty())
            error("server_name requires at least one value", line);
        for (size_t i = 0; i < args.size(); ++i)
            server.addServerName(args[i].value);
    }
    else if (name == "client_max_body_size") {
        size_t sz = 0;
        parseClientMaxBodySize(sz, args, line);
        server.setClientMaxBodySize(sz);
        _serverClientMaxBodySize = sz;
        _currentServerFlags.hasClientMaxBodySize = true;
    }
    else if (name == "error_page") {
        std::map<int, std::string> ep;
        const std::map<int, std::string>& existing = server.getErrorPages();
        for (std::map<int, std::string>::const_iterator it = existing.begin(); it != existing.end(); ++it)
            ep[it->first] = it->second;
        parseErrorPage(ep, args, line);
        for (std::map<int, std::string>::const_iterator it = ep.begin(); it != ep.end(); ++it)
            server.addErrorPage(it->first, it->second);
        _serverErrorPages = ep;
    }
    else if (name == "root") {
        if (args.size() != 1)
            error("root takes exactly one argument", line);
        _serverRoot = args[0].value;
        _currentServerFlags.hasRoot = true;
    }
    else if (name == "index") {
        if (args.size() != 1)
            error("index takes exactly one argument", line);
        _serverIndex = args[0].value;
        _currentServerFlags.hasIndex = true;
    }
    else if (name == "autoindex") {
        bool b = false;
        parseOnOff(b, args, line);
        _serverAutoindex = b;
        _currentServerFlags.hasAutoindex = true;
    }
    else {
        error("unknown server directive '" + name + "'", line);
    }
}

// location <path> { ... }
LocationConfig ConfigParser::parseLocationBlock() {
    if (current().type != Token::WORD)
        error("expected path after 'location'", current().line);

    std::string path     = current().value;
    size_t      pathLine = current().line;
    if (path.empty() || path[0] != '/')
        error("location path must start with '/'", pathLine);
    advance();

    expect(Token::OPEN_BRACE, "'{' after location path");

    LocationConfig loc;
    loc.setPath(path);
    loc.setIndex("index.html");
    loc.setAutoindex(false);
    loc.addAllowedMethod("GET"); // default: GET only

    _currentLocFlags = LocationFlags();

    while (current().type != Token::CLOSE_BRACE && current().type != Token::END)
        parseLocationDirective(loc);
    expect(Token::CLOSE_BRACE, "'}' to close location block");

    return loc;
}

void ConfigParser::parseLocationDirective(LocationConfig& location) {
    if (current().type != Token::WORD)
        error("expected directive name in location, got '" + current().value + "'", current().line);

    std::string name = current().value;
    size_t      line = current().line;
    advance();
    std::vector<Token> args = collectDirectiveArgs();

    if (name == "root") {
        if (args.size() != 1)
            error("root takes exactly one argument", line);
        location.setRoot(args[0].value);
        _currentLocFlags.hasRoot = true;
    }
    else if (name == "index") {
        if (args.size() != 1)
            error("index takes exactly one argument", line);
        location.setIndex(args[0].value);
        _currentLocFlags.hasIndex = true;
    }
    else if (name == "autoindex") {
        bool b = false;
        parseOnOff(b, args, line);
        location.setAutoindex(b);
        _currentLocFlags.hasAutoindex = true;
    }
    else if (name == "allow_methods" || name == "allowed_methods" || name == "methods") {
        parseAllowMethods(location, args, line);
    }
    else if (name == "return" || name == "redirect") {
        parseReturn(location, args, line);
    }
    else if (name == "client_max_body_size") {
        size_t sz = 0;
        parseClientMaxBodySize(sz, args, line);
        location.setClientMaxBodySize(sz);
        _currentLocFlags.hasClientMaxBodySize = true;
        _currentLocFlags.clientMaxBodySize    = sz;
    }
    else if (name == "upload_store") {
        if (args.size() != 1)
            error("upload_store takes exactly one argument", line);
        location.setUploadStore(args[0].value);
        location.setUploadEnabled(true);
    }
    else if (name == "upload_enable" || name == "upload_enabled") {
        bool b = false;
        parseOnOff(b, args, line); // same on/off grammar
        location.setUploadEnabled(b);
    }
    else if (name == "cgi_pass" || name == "cgi" || name == "cgi_extension") {
        parseCgiPass(location, args, line);
    }
    else if (name == "error_page") {
        std::map<int, std::string> tmp;
        parseErrorPage(tmp, args, line);
        for (std::map<int, std::string>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
            location.addErrorPage(it->first, it->second);
        _currentLocFlags.hasErrorPage = true;
    }
    else {
        error("unknown location directive '" + name + "'", line);
    }
}

// listen: <port> | <host:port> | <host>
void ConfigParser::parseListen(ServerConfig& server, const std::vector<Token>& args, size_t line) {
    if (args.size() != 1)
        error("listen takes exactly one argument", line);

    const std::string& v = args[0].value;
    std::string::size_type colon = v.find(':');

    if (colon == std::string::npos) {
        int port = 0;
        if (StringUtils::toInt(v, port)) {
            if (port <= 0 || port > 65535)
                error("listen: port out of range (1-65535)", line);
            server.setPort(port);
        } else {
            server.setHost(v);
        }
        return;
    }

    std::string host    = v.substr(0, colon);
    std::string portStr = v.substr(colon + 1);
    if (host.empty() || portStr.empty())
        error("listen: malformed host:port '" + v + "'", line);

    int port = 0;
    if (!StringUtils::toInt(portStr, port))
        error("listen: invalid port '" + portStr + "'", line);
    if (port <= 0 || port > 65535)
        error("listen: port out of range (1-65535)", line);

    server.setHost(host);
    server.setPort(port);
}

// error_page <code> [<code> ...] <path>;
void ConfigParser::parseErrorPage(std::map<int, std::string>& errorPages, const std::vector<Token>& args, size_t line) {
    if (args.size() < 2)
        error("error_page needs at least one code and a path", line);

    const std::string& path = args.back().value;
    if (path.empty() || path[0] != '/')
        error("error_page: path must start with '/'", line);

    for (size_t i = 0; i < args.size() - 1; ++i) {
        int code = 0;
        if (!StringUtils::toInt(args[i].value, code))
            error("error_page: '" + args[i].value + "' is not a number", line);
        if (code < 300 || code > 599)
            error("error_page: code must be in 300-599", line);
        errorPages[code] = path;
    }
}

void ConfigParser::parseClientMaxBodySize(size_t& target, const std::vector<Token>& args, size_t line) {
    if (args.size() != 1)
        error("client_max_body_size takes exactly one argument", line);
    if (!parseSize(args[0].value, target))
        error("client_max_body_size: invalid size '" + args[0].value + "'", line);
}

void ConfigParser::parseOnOff(bool& target, const std::vector<Token>& args, size_t line) {
    if (args.size() != 1)
        error("on/off directive takes exactly one argument", line);
    std::string v = StringUtils::toLower(args[0].value);
    if (v == "on")
        target = true;
    else if (v == "off")
        target = false;
    else
        error("expected 'on' or 'off', got '" + args[0].value + "'", line);
}

void ConfigParser::parseReturn(LocationConfig& location, const std::vector<Token>& args, size_t line) {
    int         code = 302;
    std::string target;

    if (args.size() == 1) {
        target = args[0].value;
    } else if (args.size() == 2) {
        if (!StringUtils::toInt(args[0].value, code))
            error("return: '" + args[0].value + "' is not a number", line);
        if (code < 300 || code > 399)
            error("return: redirect code must be in 300-399", line);
        target = args[1].value;
    } else {
        error("return takes 1 or 2 arguments (code? url)", line);
    }

    if (target.empty())
        error("return: empty target", line);
    location.setRedirect(target, code);
}

void ConfigParser::parseAllowMethods(LocationConfig& location, const std::vector<Token>& args, size_t line) {
    if (args.empty())
        error("allow_methods requires at least one method", line);

    std::vector<std::string> methods;
    for (size_t i = 0; i < args.size(); ++i) {
        std::string m = StringUtils::toUpper(args[i].value);
        if (!isHttpMethod(m))
            error("allow_methods: unsupported method '" + args[i].value + "'", line);
        methods.push_back(m);
    }
    location.setAllowedMethods(methods);
}

// cgi_pass <ext> <interpreter>;
void ConfigParser::parseCgiPass(LocationConfig& location, const std::vector<Token>& args, size_t line) {
    if (args.size() != 2)
        error("cgi_pass takes exactly two arguments: <ext> <interpreter>", line);
    std::string ext         = args[0].value;
    std::string interpreter = args[1].value;
    if (ext.empty() || ext[0] != '.')
        error("cgi_pass: extension must start with '.' (e.g. .py)", line);
    if (access(interpreter.c_str(), X_OK) != 0)
        Logger::warn("cgi_pass: interpreter '" + interpreter + "' is not found or not executable (will fail at runtime)");
    else
        Logger::debug("  CGI interpreter verified: " + ext + " -> " + interpreter);
    location.addCgiHandler(ext, interpreter);
}

// Validation
void ConfigParser::validateConfig(const Config& config) const {
    if (config.empty())
        throw ParseError("Configuration is empty: at least one server block is required.");

    typedef std::pair<std::string, int> HostPort;
    std::set<HostPort> seen;
    const std::vector<ServerConfig>& servers = config.getServers();
    for (size_t i = 0; i < servers.size(); ++i) {
        validateServer(servers[i]);
        HostPort hp = std::make_pair(servers[i].getHost(), servers[i].getPort());
        if (!seen.insert(hp).second)
            Logger::warn("Multiple servers on " + servers[i].getHost() + ":" + StringUtils::toString(servers[i].getPort()) + " - dispatch will use Host header (virtual hosting).");
    }
}

void ConfigParser::validateServer(const ServerConfig& server) const {
    if (server.getPort() <= 0 || server.getPort() > 65535)
        throw ParseError("Server has invalid port " + StringUtils::toString(server.getPort()));

    const std::vector<LocationConfig>& locs = server.getLocations();
    if (locs.empty())
        throw ParseError("Server on port " + StringUtils::toString(server.getPort()) + " has no location block (at least 'location /' is required).");

    std::set<std::string> seenPaths;
    for (size_t i = 0; i < locs.size(); ++i) {
        validateLocation(locs[i]);
        if (!seenPaths.insert(locs[i].getPath()).second)
            throw ParseError("Server on port " + StringUtils::toString(server.getPort()) + " has duplicate location '" + locs[i].getPath() + "'.");
    }
}

void ConfigParser::validateLocation(const LocationConfig& location) const {
    if (location.getAllowedMethods().empty())
        throw ParseError("Location '" + location.getPath() + "' has no allowed methods.");

    if (location.getUploadEnabled() && location.getUploadStore().empty())
        throw ParseError("Location '" + location.getPath() + "' enables upload but has no upload_store.");

    if (!location.hasRedirect() && location.getCgiHandlers().empty() && location.getRoot().empty())
        throw ParseError("Location '" + location.getPath() + "' has no root, no redirect and no CGI handler -- nothing to serve.");
}

// Token-stream helpers
const ConfigParser::Token& ConfigParser::current() const {
    return _tokens[_pos];
}

void ConfigParser::advance() {
    if (_tokens[_pos].type != Token::END)
        ++_pos;
}

void ConfigParser::expect(Token::Type type, const std::string& what) {
    if (current().type != type)
        error("expected " + what + ", got '" + current().value + "'", current().line);
    advance();
}

// Error helper
void ConfigParser::error(const std::string& msg, size_t line) const {
    std::ostringstream oss;
    oss << _filepath << ":" << line << ": " << msg;
    throw ParseError(oss.str());
}

// Misc parse helpers
std::vector<ConfigParser::Token> ConfigParser::collectDirectiveArgs() {
    std::vector<Token> args;
    while (current().type != Token::SEMICOLON) {
        if (current().type == Token::END || current().type == Token::OPEN_BRACE || current().type == Token::CLOSE_BRACE)
            error("expected ';' to terminate directive", current().line);
        args.push_back(current());
        advance();
    }
    advance(); // consume the ';'
    return args;
}

// Accepts <digits>[k|K|m|M|g|G|b|B]
bool ConfigParser::parseSize(const std::string& s, size_t& out) const {
    if (s.empty()) return false;

    std::string digits = s;
    size_t      mult   = 1;
    char        suffix = s[s.size() - 1];

    if (!std::isdigit(static_cast<unsigned char>(suffix))) {
        switch (suffix) {
            case 'k': case 'K': mult = 1024UL; break;
            case 'm': case 'M': mult = 1024UL * 1024UL; break;
            case 'g': case 'G': mult = 1024UL * 1024UL * 1024UL; break;
            case 'b': case 'B': mult = 1; break;
            default: return false;
        }
        digits = s.substr(0, s.size() - 1);
    }
    if (digits.empty()) return false;
    for (size_t i = 0; i < digits.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(digits[i])))
            return false;
    }

    size_t base = 0;
    if (!StringUtils::toSize(digits, base))
        return false;

    if (mult != 0 && base > static_cast<size_t>(-1) / mult)
        return false;
    out = base * mult;
    return true;
}

bool ConfigParser::isHttpMethod(const std::string& m) const {
    return m == "GET" || m == "POST" || m == "DELETE";
}
