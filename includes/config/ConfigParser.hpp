#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

# include <string>
# include <vector>
# include <map>
# include <stdexcept>
# include "config/Config.hpp"

// ConfigParser
//
// Reads an nginx-like configuration file and produces a fully resolved
// Config object. Grammar:
//
//     config        := server_block+
//     server_block  := "server" "{" server_directive* "}"
//     location_block:= "location" path "{" location_directive* "}"
//
// Server-level directives:
//   listen <host:port | port | host>;
//   server_name <name> [<name> ...];
//   client_max_body_size <size>;          // 100 / 1k / 1m / 1g
//   error_page <code> [<code> ...] <path>;
//   root <path>;                          // inherited by locations
//   index <file>;                         // inherited by locations
//   autoindex <on|off>;                   // inherited by locations
//   location <path> { ... }
//
// Location-level directives:
//   root <path>;
//   index <file>;
//   autoindex <on|off>;
//   allow_methods <m1> [<m2> ...];        // GET / POST / DELETE
//   return <code> <url>;
//   client_max_body_size <size>;
//   upload_store <path>;
//   upload_enable <on|off>;
//   cgi_pass <ext> <interpreter>;         // e.g. .py /usr/bin/python3
//   error_page <code> [<code> ...] <path>;
//
// On any syntactic or semantic problem a ParseError is thrown with the
// offending line number embedded in the message.
class ConfigParser {
public:
    class ParseError : public std::runtime_error {
    public:
        explicit ParseError(const std::string& msg);
    };

    ConfigParser();
    ~ConfigParser();

    Config parse(const std::string& filepath);

private:
    ConfigParser(const ConfigParser& other);
    ConfigParser& operator=(const ConfigParser& other);

	    struct Token {
        enum Type {
            WORD,
            OPEN_BRACE,
            CLOSE_BRACE,
            SEMICOLON,
            END
        };
        Type        type;
        std::string value;
        size_t      line;

        Token() : type(END), value(), line(0) {}
        Token(Type t, const std::string& v, size_t l)
            : type(t), value(v), line(l) {}
    };

    std::string         readFile(const std::string& filepath) const;
    std::vector<Token>  tokenize(const std::string& source) const;
    Config              buildConfig();

    ServerConfig    parseServerBlock();
    void            parseServerDirective(ServerConfig& server);
    LocationConfig  parseLocationBlock();
    void            parseLocationDirective(LocationConfig& location);

    void parseListen(ServerConfig& server, const std::vector<Token>& args, size_t line);
    void parseErrorPage(std::map<int, std::string>& errorPages, const std::vector<Token>& args, size_t line);
    void parseClientMaxBodySize(size_t& target, const std::vector<Token>& args, size_t line);
    void parseOnOff(bool& target, const std::vector<Token>& args, size_t line);
    void parseReturn(LocationConfig& location, const std::vector<Token>& args, size_t line);
    void parseAllowMethods(LocationConfig& location, const std::vector<Token>& args, size_t line);
    void parseCgiPass(LocationConfig& location, const std::vector<Token>& args, size_t line);

    void validateConfig(const Config& config) const;
    void validateServer(const ServerConfig& server) const;
    void validateLocation(const LocationConfig& location) const;

    const Token& current() const;
    void         advance();
    void         expect(Token::Type type, const std::string& what);

    void error(const std::string& msg, size_t line) const;

    bool                parseSize(const std::string& s, size_t& out) const;
    std::vector<Token>  collectDirectiveArgs();
    bool                isHttpMethod(const std::string& m) const;

    std::vector<Token>  _tokens;
    size_t              _pos;
    std::string         _filepath;

    struct LocationFlags {
        bool hasRoot;
        bool hasIndex;
        bool hasAutoindex;
        bool hasClientMaxBodySize;
        bool hasErrorPage;
        size_t clientMaxBodySize;

        LocationFlags()
            : hasRoot(false), hasIndex(false), hasAutoindex(false),
              hasClientMaxBodySize(false), hasErrorPage(false),
              clientMaxBodySize(0) {}
    };
    LocationFlags _currentLocFlags;

    struct ServerFlags {
        bool hasListen;
        bool hasRoot;
        bool hasIndex;
        bool hasAutoindex;
        bool hasClientMaxBodySize;

        ServerFlags()
            : hasListen(false), hasRoot(false), hasIndex(false),
              hasAutoindex(false), hasClientMaxBodySize(false) {}
    };
    ServerFlags _currentServerFlags;

    std::string                 _serverRoot;
    std::string                 _serverIndex;
    bool                        _serverAutoindex;
    size_t                      _serverClientMaxBodySize;
    std::map<int, std::string>  _serverErrorPages;
    std::vector<LocationFlags>  _allLocationFlags;     // one per parsed location
};

#endif
