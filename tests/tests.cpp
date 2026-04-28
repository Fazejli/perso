// tests/test_all.cpp
//
// Unified test suite for HttpRequest parser and ConfigParser.
// Output: grouped by section, one line per check, color-coded,
// with a final summary per group and overall.

#include "http/HttpRequest.hpp"
#include "config/ConfigParser.hpp"
#include "config/Config.hpp"
#include "config/ServerConfig.hpp"
#include "config/LocationConfig.hpp"
#include "utils/Logger.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>

// =============================================================================
// Mini test framework
// =============================================================================

static int g_total_pass = 0;
static int g_total_fail = 0;
static int g_sect_pass  = 0;
static int g_sect_fail  = 0;

static const char* RST = "\033[0m";
static const char* GRN = "\033[0;32m";
static const char* RED = "\033[0;31m";
static const char* CYN = "\033[0;36m";
static const char* DIM = "\033[0;90m";
static const char* BLD = "\033[1m";

#define CHECK(cond, desc) do { \
    if (cond) { ++g_total_pass; ++g_sect_pass; \
        std::cout << "    " << GRN << "✓ " << RST << DIM << desc << RST << "\n"; \
    } else { ++g_total_fail; ++g_sect_fail; \
        std::cout << "    " << RED << "✗ " << desc << RST << "\n"; \
    } \
} while (0)

static void section(const std::string& name) {
    g_sect_pass = 0;
    g_sect_fail = 0;
    std::cout << "\n  " << CYN << "● " << RST << BLD << name << RST << "\n";
}

static void section_end() {
    if (g_sect_fail > 0)
        std::cout << "    " << RED << "→ " << g_sect_fail << " failed" << RST << "\n";
}

static void group(const std::string& name) {
    std::cout << "\n" << BLD << "━━━ " << name << " ━━━" << RST << "\n";
}

// Helper: write a string to a temp file, return path
static std::string writeTmpConf(const std::string& content) {
    std::string path = "/tmp/webserv_test.conf";
    std::ofstream f(path.c_str());
    f << content;
    f.close();
    return path;
}

// Helper: try to parse a config string, return true if it succeeded
static bool configParses(const std::string& content) {
    try {
        ConfigParser p;
        p.parse(writeTmpConf(content));
        return true;
    } catch (...) {
        return false;
    }
}

// Helper: try to parse a config file, return true if it succeeded
static bool configFileParses(const std::string& path) {
    try {
        ConfigParser p;
        p.parse(path);
        return true;
    } catch (...) {
        return false;
    }
}

// =============================================================================
//  HTTP REQUEST PARSER TESTS
// =============================================================================

// ---- Basic parsing ----------------------------------------------------------

static void test_simple_get() {
    section("Simple GET, single append");
    HttpRequest req;
    std::string raw = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: test\r\n\r\n";
    bool done = req.appendData(raw);
    CHECK(done, "complete after single append");
    CHECK(req.isComplete(), "state is COMPLETE");
    CHECK(!req.hasError(), "no error");
    CHECK(req.getMethod() == "GET", "method = GET");
    CHECK(req.getUri() == "/index.html", "uri = /index.html");
    CHECK(req.getPath() == "/index.html", "path = /index.html");
    CHECK(req.getQueryString().empty(), "empty query string");
    CHECK(req.getVersion() == "HTTP/1.1", "version = HTTP/1.1");
    CHECK(req.getHeader("host") == "example.com", "host header (lowercase lookup)");
    CHECK(req.getHeader("Host") == "example.com", "host header (mixed case lookup)");
    CHECK(req.getHeader("User-Agent") == "test", "user-agent header");
    CHECK(req.hasHeader("HOST"), "hasHeader case-insensitive");
    CHECK(req.getBody().empty(), "no body");
    section_end();
}

static void test_query_string() {
    section("URI with query string");
    HttpRequest req;
    req.appendData("GET /search?q=hello&lang=fr HTTP/1.1\r\nHost: x\r\n\r\n");
    CHECK(req.isComplete(), "complete");
    CHECK(req.getPath() == "/search", "path = /search");
    CHECK(req.getQueryString() == "q=hello&lang=fr", "query string parsed");
    CHECK(req.getUri() == "/search?q=hello&lang=fr", "full URI preserved");
    section_end();
}

static void test_post_content_length() {
    section("POST with Content-Length body");
    HttpRequest req;
    req.appendData("POST /submit HTTP/1.1\r\nHost: x\r\nContent-Length: 11\r\nContent-Type: text/plain\r\n\r\nhello world");
    CHECK(req.isComplete(), "complete");
    CHECK(req.getMethod() == "POST", "method = POST");
    CHECK(req.getBody() == "hello world", "body captured");
    section_end();
}

static void test_delete_method() {
    section("DELETE method");
    HttpRequest req;
    req.appendData("DELETE /resource HTTP/1.1\r\nHost: a\r\n\r\n");
    CHECK(req.isComplete(), "complete");
    CHECK(req.getMethod() == "DELETE", "method = DELETE");
    section_end();
}

// ---- Incremental / byte-by-byte --------------------------------------------

static void test_byte_by_byte() {
    section("Byte-by-byte feeding");
    HttpRequest req;
    std::string raw = "POST /up HTTP/1.1\r\nHost: y\r\nContent-Length: 5\r\n\r\nABCDE";
    bool done = false;
    for (size_t i = 0; i < raw.size() && !done; ++i) {
        done = req.appendData(std::string(1, raw[i]));
        if (!done) CHECK(!req.hasError(), "no error mid-stream");
    }
    CHECK(done, "complete after last byte");
    CHECK(req.getBody() == "ABCDE", "body correct");
    section_end();
}

static void test_split_at_request_line() {
    section("Buffer split mid-request-line");
    HttpRequest req;
    CHECK(!req.appendData("GET /pa"), "partial 1");
    CHECK(!req.appendData("th HTTP/1.1\r\nHost: "), "partial 2");
    CHECK(req.appendData("a\r\n\r\n"), "now complete");
    CHECK(req.getPath() == "/path", "path reconstructed");
    section_end();
}

static void test_split_at_header() {
    section("Buffer split inside header line");
    HttpRequest req;
    req.appendData("GET / HTTP/1.1\r\nHost: exam");
    CHECK(req.getState() == HttpRequest::PARSE_HEADERS, "in PARSE_HEADERS");
    req.appendData("ple.com\r\n\r\n");
    CHECK(req.isComplete(), "complete");
    CHECK(req.getHeader("host") == "example.com", "host reassembled");
    section_end();
}

static void test_split_header_body() {
    section("Buffer split between headers and body");
    HttpRequest req;
    req.appendData("POST / HTTP/1.1\r\nHost: a\r\nContent-Length: 10\r\n\r\n");
    CHECK(!req.isComplete(), "not yet complete");
    CHECK(req.getState() == HttpRequest::PARSE_BODY, "in PARSE_BODY");
    req.appendData("0123456789");
    CHECK(req.isComplete(), "complete after body arrives");
    CHECK(req.getBody() == "0123456789", "body correct");
    section_end();
}

// ---- Chunked transfer encoding ---------------------------------------------

static void test_chunked_simple() {
    section("Chunked simple");
    HttpRequest req;
    req.appendData("POST /c HTTP/1.1\r\nHost: z\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n");
    CHECK(req.isComplete(), "complete");
    CHECK(req.getBody() == "hello world", "dechunked body");
    section_end();
}

static void test_chunked_extensions() {
    section("Chunked with extensions (ignored)");
    HttpRequest req;
    req.appendData("POST /c HTTP/1.1\r\nHost: z\r\nTransfer-Encoding: chunked\r\n\r\n4;foo=bar\r\nabcd\r\n0\r\n\r\n");
    CHECK(req.isComplete(), "complete");
    CHECK(req.getBody() == "abcd", "body ok");
    section_end();
}

static void test_chunked_byte_by_byte() {
    section("Chunked byte-by-byte");
    HttpRequest req;
    std::string raw = "POST /c HTTP/1.1\r\nHost: z\r\nTransfer-Encoding: chunked\r\n\r\na\r\n0123456789\r\n5\r\nABCDE\r\n0\r\n\r\n";
    bool done = false;
    for (size_t i = 0; i < raw.size() && !done; ++i)
        done = req.appendData(std::string(1, raw[i]));
    CHECK(done, "complete");
    CHECK(req.getBody() == "0123456789ABCDE", "body assembled");
    section_end();
}

static void test_chunked_trailers() {
    section("Chunked with trailers (discarded)");
    HttpRequest req;
    req.appendData("POST /c HTTP/1.1\r\nHost: z\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n0\r\nX-Trailer: ignored\r\n\r\n");
    CHECK(req.isComplete(), "complete");
    CHECK(req.getBody() == "abc", "body ok");
    CHECK(!req.hasHeader("x-trailer"), "trailer discarded");
    section_end();
}

// ---- Error cases ------------------------------------------------------------

static void test_malformed_request_line() {
    section("Malformed request line (400)");
    { HttpRequest r; r.appendData("GET\r\n\r\n");
      CHECK(r.hasError() && r.getErrorCode() == 400, "no spaces → 400"); }
    { HttpRequest r; r.appendData("GET /  HTTP/1.1\r\n\r\n");
      CHECK(r.hasError() && r.getErrorCode() == 400, "double space → 400"); }
    { HttpRequest r; r.appendData("GET / HTTP/1.1 extra\r\n\r\n");
      CHECK(r.hasError(), "extra token → error"); }
    section_end();
}

static void test_unsupported_method() {
    section("Unsupported method (501)");
    HttpRequest req;
    req.appendData("PUT /x HTTP/1.1\r\nHost: a\r\n\r\n");
    CHECK(req.hasError(), "PUT rejected");
    CHECK(req.getErrorCode() == 501, "501 Not Implemented");
    section_end();
}

static void test_invalid_method_char() {
    section("Invalid method chars (400)");
    HttpRequest req;
    req.appendData("G@T / HTTP/1.1\r\nHost: a\r\n\r\n");
    CHECK(req.hasError() && req.getErrorCode() == 400, "bad char → 400");
    section_end();
}

static void test_bad_uri() {
    section("Bad URI (400)");
    { HttpRequest r; r.appendData("GET http://example.com/ HTTP/1.1\r\nHost: a\r\n\r\n");
      CHECK(r.hasError(), "absolute-form rejected"); }
    { HttpRequest r; r.appendData("GET foo HTTP/1.1\r\nHost: a\r\n\r\n");
      CHECK(r.hasError(), "no leading / rejected"); }
    section_end();
}

static void test_bad_version() {
    section("Bad HTTP version");
    { HttpRequest r; r.appendData("GET / HTTP/2.0\r\nHost: a\r\n\r\n");
      CHECK(r.hasError() && r.getErrorCode() == 505, "HTTP/2.0 → 505"); }
    { HttpRequest r; r.appendData("GET / FTP/1.0\r\nHost: a\r\n\r\n");
      CHECK(r.hasError() && r.getErrorCode() == 400, "FTP/1.0 → 400"); }
    section_end();
}

static void test_missing_host() {
    section("HTTP/1.1 without Host (400)");
    HttpRequest req;
    req.appendData("GET / HTTP/1.1\r\nUser-Agent: x\r\n\r\n");
    CHECK(req.hasError() && req.getErrorCode() == 400, "missing Host → 400");
    section_end();
}

static void test_http10_no_host() {
    section("HTTP/1.0 without Host (ok)");
    HttpRequest req;
    req.appendData("GET / HTTP/1.0\r\n\r\n");
    CHECK(req.isComplete() && !req.hasError(), "accepted");
    section_end();
}

static void test_cl_and_te() {
    section("Content-Length + Transfer-Encoding (400)");
    HttpRequest req;
    req.appendData("POST / HTTP/1.1\r\nHost: a\r\nContent-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n");
    CHECK(req.hasError() && req.getErrorCode() == 400, "smuggling rejected");
    section_end();
}

static void test_duplicate_cl() {
    section("Duplicate Content-Length (400)");
    HttpRequest req;
    req.appendData("POST / HTTP/1.1\r\nHost: a\r\nContent-Length: 5\r\nContent-Length: 7\r\n\r\nhello");
    CHECK(req.hasError() && req.getErrorCode() == 400, "duplicate CL → 400");
    section_end();
}

static void test_duplicate_host() {
    section("Duplicate Host (400)");
    HttpRequest req;
    req.appendData("GET / HTTP/1.1\r\nHost: a\r\nHost: b\r\n\r\n");
    CHECK(req.hasError(), "duplicate Host rejected");
    section_end();
}

static void test_post_no_length() {
    section("POST without Content-Length (411)");
    HttpRequest req;
    req.appendData("POST / HTTP/1.1\r\nHost: a\r\n\r\n");
    CHECK(req.hasError() && req.getErrorCode() == 411, "411 Length Required");
    section_end();
}

static void test_unsupported_te() {
    section("Non-chunked Transfer-Encoding (501)");
    HttpRequest req;
    req.appendData("POST / HTTP/1.1\r\nHost: a\r\nTransfer-Encoding: gzip\r\n\r\n");
    CHECK(req.hasError() && req.getErrorCode() == 501, "gzip TE → 501");
    section_end();
}

static void test_invalid_headers() {
    section("Invalid header formats (400)");
    { HttpRequest r; r.appendData("GET / HTTP/1.1\r\nHost a\r\n\r\n");
      CHECK(r.hasError(), "no colon → error"); }
    { HttpRequest r; r.appendData("GET / HTTP/1.1\r\n: value\r\nHost: a\r\n\r\n");
      CHECK(r.hasError(), "empty name → error"); }
    { HttpRequest r; r.appendData("GET / HTTP/1.1\r\nHost : a\r\n\r\n");
      CHECK(r.hasError(), "space before colon → error"); }
    { HttpRequest r; r.appendData("GET / HTTP/1.1\r\nHost: a\r\n foo\r\n\r\n");
      CHECK(r.hasError(), "obsolete folding → error"); }
    section_end();
}

static void test_bad_chunk_size() {
    section("Bad chunk size (400)");
    HttpRequest req;
    req.appendData("POST / HTTP/1.1\r\nHost: a\r\nTransfer-Encoding: chunked\r\n\r\nZZZ\r\n");
    CHECK(req.hasError() && req.getErrorCode() == 400, "non-hex → 400");
    section_end();
}

static void test_missing_chunk_crlf() {
    section("Chunk missing trailing CRLF (400)");
    HttpRequest req;
    req.appendData("POST / HTTP/1.1\r\nHost: a\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabcXX");
    CHECK(req.hasError(), "bad chunk terminator");
    section_end();
}

// ---- Edge cases -------------------------------------------------------------

static void test_content_length_zero() {
    section("Content-Length: 0");
    HttpRequest req;
    req.appendData("POST / HTTP/1.1\r\nHost: a\r\nContent-Length: 0\r\n\r\n");
    CHECK(req.isComplete(), "complete");
    CHECK(req.getBody().empty(), "body empty");
    section_end();
}

static void test_duplicate_nonrestricted_header() {
    section("Duplicate non-restricted header (comma-combined)");
    HttpRequest req;
    req.appendData("GET / HTTP/1.1\r\nHost: a\r\nAccept: text/html\r\nAccept: application/json\r\n\r\n");
    CHECK(req.isComplete(), "complete");
    CHECK(req.getHeader("accept") == "text/html, application/json", "combined with ', '");
    section_end();
}

static void test_after_complete() {
    section("appendData after complete is a no-op");
    HttpRequest req;
    req.appendData("GET / HTTP/1.1\r\nHost: a\r\n\r\n");
    CHECK(req.isComplete(), "complete");
    CHECK(req.appendData("garbage") == true, "still returns true");
    CHECK(req.isComplete(), "stays complete");
    section_end();
}

static void test_after_error() {
    section("appendData after error is a no-op");
    HttpRequest req;
    req.appendData("@BAD / HTTP/1.1\r\nHost: a\r\n\r\n");
    CHECK(req.hasError(), "error");
    CHECK(req.appendData("more") == false, "returns false");
    CHECK(req.hasError(), "stays errored");
    section_end();
}

static void test_reset() {
    section("reset() clears state for reuse");
    HttpRequest req;
    req.appendData("GET / HTTP/1.1\r\nHost: a\r\n\r\n");
    CHECK(req.isComplete(), "first request complete");
    req.reset();
    CHECK(req.getState() == HttpRequest::PARSE_REQUEST_LINE, "state reset");
    CHECK(req.getMethod().empty(), "method cleared");
    req.appendData("DELETE /x HTTP/1.1\r\nHost: b\r\n\r\n");
    CHECK(req.isComplete() && req.getMethod() == "DELETE", "second request works");
    section_end();
}

static void test_case_insensitive_headers() {
    section("Case-insensitive header lookup");
    HttpRequest req;
    req.appendData("GET / HTTP/1.1\r\nHOST: val\r\nContent-Length: 0\r\nx-Custom-Header: val\r\n\r\n");
    CHECK(req.hasHeader("host") && req.hasHeader("Host") && req.hasHeader("HOST"), "any case works");
    CHECK(req.getHeader("X-CUSTOM-HEADER") == "val", "arbitrary case lookup");
    section_end();
}

static void test_trimmed_header_values() {
    section("Header values trimmed");
    HttpRequest req;
    req.appendData("GET / HTTP/1.1\r\nHost:   example.com   \r\nX-Foo:\tbar\t\r\n\r\n");
    CHECK(req.getHeader("host") == "example.com", "host trimmed");
    CHECK(req.getHeader("x-foo") == "bar", "x-foo trimmed");
    section_end();
}

static void test_request_line_too_long() {
    section("Request line too long (414)");
    HttpRequest req;
    std::string huge = "GET /";
    huge.append(10000, 'a');
    huge += " HTTP/1.1\r\nHost: a\r\n\r\n";
    req.appendData(huge);
    CHECK(req.hasError() && req.getErrorCode() == 414, "oversize → 414");
    section_end();
}

static void test_leading_crlf() {
    section("Leading CRLF before request line (400)");
    HttpRequest req;
    req.appendData("\r\nGET / HTTP/1.1\r\nHost: a\r\n\r\n");
    CHECK(req.hasError() && req.getErrorCode() == 400, "leading CRLF → 400");
    section_end();
}

static void test_negative_cl() {
    section("Negative Content-Length (400)");
    HttpRequest req;
    req.appendData("POST / HTTP/1.1\r\nHost: a\r\nContent-Length: -1\r\n\r\n");
    CHECK(req.hasError(), "rejected");
    section_end();
}

static void test_nonnumeric_cl() {
    section("Non-numeric Content-Length (400)");
    HttpRequest req;
    req.appendData("POST / HTTP/1.1\r\nHost: a\r\nContent-Length: abc\r\n\r\n");
    CHECK(req.hasError(), "rejected");
    section_end();
}

static void test_large_body() {
    section("1 MiB body in a single append");
    HttpRequest req;
    std::string body(1024 * 1024, 'x');
    std::string raw = "POST / HTTP/1.1\r\nHost: a\r\nContent-Length: 1048576\r\n\r\n" + body;
    CHECK(req.appendData(raw), "complete");
    CHECK(req.getBody().size() == 1048576, "body size correct");
    section_end();
}

// =============================================================================
//  CONFIG PARSER TESTS
// =============================================================================

// ---- Valid configs (inline) -------------------------------------------------

static void test_cfg_minimal() {
    section("Minimal valid config");
    CHECK(configParses("server { listen 8080; location / { root /var/www; } }"), "parses ok");
    section_end();
}

static void test_cfg_full_featured() {
    section("Full-featured config");
    bool ok = configParses(
        "server {\n"
        "    listen 127.0.0.1:8080;\n"
        "    server_name example.com www.example.com;\n"
        "    client_max_body_size 2m;\n"
        "    root /var/www;\n"
        "    index index.html;\n"
        "    autoindex off;\n"
        "    error_page 404 /errors/404.html;\n"
        "    error_page 500 502 503 /errors/5xx.html;\n"
        "    location / { allow_methods GET POST; }\n"
        "    location /upload { allow_methods POST; upload_store /tmp/up; client_max_body_size 100m; }\n"
        "    location /old { return 301 /new; }\n"
        "    location /cgi { root /var/cgi; allow_methods GET POST; cgi_pass .py /usr/bin/python3; }\n"
        "}\n");
    CHECK(ok, "parses ok");
    section_end();
}

static void test_cfg_virtual_hosts() {
    section("Multiple servers (virtual hosting)");
    bool ok = configParses(
        "server { listen 8080; server_name a.com; location / { root /a; } }\n"
        "server { listen 8080; server_name b.com; location / { root /b; } }\n");
    CHECK(ok, "parses ok (same port, different names)");
    section_end();
}

static void test_cfg_inheritance() {
    section("Server → location inheritance");
    std::string conf =
        "server {\n"
        "    listen 80;\n"
        "    root /srv;\n"
        "    index home.html;\n"
        "    autoindex on;\n"
        "    client_max_body_size 5m;\n"
        "    error_page 404 /e404.html;\n"
        "    error_page 500 /e500.html;\n"
        "    location / { allow_methods GET; }\n"
        "    location /api { allow_methods POST; error_page 404 /api404.html; }\n"
        "}\n";
    ConfigParser p;
    Config c = p.parse(writeTmpConf(conf));
    const std::vector<LocationConfig>& locs = c.getServers()[0].getLocations();

    CHECK(locs[0].getRoot() == "/srv", "/ inherits root");
    CHECK(locs[0].getIndex() == "home.html", "/ inherits index");
    CHECK(locs[0].getAutoindex() == true, "/ inherits autoindex");
    CHECK(locs[0].getClientMaxBodySize() == 5 * 1024 * 1024, "/ inherits cmbs");
    CHECK(locs[0].getErrorPagePath(404) == "/e404.html", "/ inherits error_page 404");
    CHECK(locs[0].getErrorPagePath(500) == "/e500.html", "/ inherits error_page 500");

    CHECK(locs[1].getRoot() == "/srv", "/api inherits root");
    CHECK(locs[1].getErrorPagePath(404) == "/api404.html", "/api overrides error_page 404");
    CHECK(locs[1].getErrorPagePath(500) == "/e500.html", "/api inherits error_page 500");
    section_end();
}

static void test_cfg_cmbs_zero() {
    section("client_max_body_size 0 (allowed)");
    bool ok = configParses("server { listen 80; client_max_body_size 0; location / { root /x; } }");
    CHECK(ok, "parses ok");
    ConfigParser p;
    Config c = p.parse(writeTmpConf("server { listen 80; client_max_body_size 0; location / { root /x; } }"));
    CHECK(c.getServers()[0].getClientMaxBodySize() == 0, "server cmbs = 0");
    section_end();
}

static void test_cfg_size_suffixes() {
    section("Size suffixes (k/m/g)");
    ConfigParser p;
    Config c = p.parse(writeTmpConf(
        "server { listen 80; location / { root /x; client_max_body_size 10k; } }\n"));
    CHECK(c.getServers()[0].getLocations()[0].getClientMaxBodySize() == 10240, "10k = 10240");

    Config c2 = p.parse(writeTmpConf(
        "server { listen 80; location / { root /x; client_max_body_size 2m; } }\n"));
    CHECK(c2.getServers()[0].getLocations()[0].getClientMaxBodySize() == 2 * 1024 * 1024, "2m = 2097152");

    Config c3 = p.parse(writeTmpConf(
        "server { listen 80; location / { root /x; client_max_body_size 1g; } }\n"));
    CHECK(c3.getServers()[0].getLocations()[0].getClientMaxBodySize() == 1024UL * 1024 * 1024, "1g = 1073741824");
    section_end();
}

static void test_cfg_comments_and_quotes() {
    section("Comments and quoted strings");
    bool ok = configParses(
        "# full line comment\n"
        "server {\n"
        "    listen 80; # inline comment\n"
        "    server_name 'my server';\n"
        "    location / { root \"/var/www/my site\"; }\n"
        "}\n");
    CHECK(ok, "comments and quotes accepted");
    section_end();
}

static void test_cfg_directive_aliases() {
    section("Directive aliases");
    CHECK(configParses("server { listen 80; location / { root /x; allowed_methods GET; } }"), "allowed_methods ok");
    CHECK(configParses("server { listen 80; location / { root /x; methods GET; } }"), "methods ok");
    CHECK(configParses("server { listen 80; location / { root /x; } location /old { redirect /new; } }"), "redirect ok");
    CHECK(configParses("server { listen 80; location / { root /x; cgi_extension .py /usr/bin/python3; } }"), "cgi_extension ok");
    CHECK(configParses("server { listen 80; location / { root /x; upload_enabled off; } }"), "upload_enabled ok");
    section_end();
}

// ---- Invalid configs (inline) -----------------------------------------------

static void test_cfg_empty() {
    section("Empty config → error");
    CHECK(!configParses(""), "empty string rejected");
    CHECK(!configParses("# only comments\n"), "only comments rejected");
    section_end();
}

static void test_cfg_bad_port() {
    section("Bad port → error");
    CHECK(!configParses("server { listen 99999; location / { root /x; } }"), "port 99999 rejected");
    CHECK(!configParses("server { listen 0; location / { root /x; } }"), "port 0 rejected");
    CHECK(!configParses("server { listen -1; location / { root /x; } }"), "port -1 rejected");
    section_end();
}

static void test_cfg_no_location() {
    section("Server without location → error");
    CHECK(!configParses("server { listen 80; }"), "no location rejected");
    section_end();
}

static void test_cfg_bad_location_path() {
    section("Location path without / → error");
    CHECK(!configParses("server { listen 80; location bad { root /x; } }"), "path 'bad' rejected");
    section_end();
}

static void test_cfg_unknown_directive() {
    section("Unknown directive → error");
    CHECK(!configParses("server { listen 80; foobar on; location / { root /x; } }"), "server-level rejected");
    CHECK(!configParses("server { listen 80; location / { root /x; foobar on; } }"), "location-level rejected");
    section_end();
}

static void test_cfg_missing_semicolon() {
    section("Missing semicolon → error");
    CHECK(!configParses("server { listen 80\n location / { root /x; } }"), "rejected");
    section_end();
}

static void test_cfg_missing_brace() {
    section("Missing closing brace → error");
    CHECK(!configParses("server { listen 80; location / { root /x; }"), "rejected");
    section_end();
}

static void test_cfg_bad_method() {
    section("Unsupported method in allow_methods → error");
    CHECK(!configParses("server { listen 80; location / { root /x; allow_methods PATCH; } }"), "PATCH rejected");
    section_end();
}

static void test_cfg_bad_size() {
    section("Invalid size format → error");
    CHECK(!configParses("server { listen 80; client_max_body_size 5x; location / { root /x; } }"), "5x rejected");
    CHECK(!configParses("server { listen 80; client_max_body_size abc; location / { root /x; } }"), "abc rejected");
    section_end();
}

static void test_cfg_nothing_to_serve() {
    section("Location with no root/redirect/cgi → error");
    CHECK(!configParses("server { listen 80; location / { allow_methods GET; } }"), "rejected");
    section_end();
}

static void test_cfg_duplicate_location() {
    section("Duplicate location path → error");
    CHECK(!configParses("server { listen 80; location / { root /a; } location / { root /b; } }"), "rejected");
    section_end();
}

static void test_cfg_nested_server() {
    section("Nested server block → error");
    CHECK(!configParses("server { listen 80; server { listen 90; } location / { root /x; } }"), "rejected");
    section_end();
}

static void test_cfg_bad_on_off() {
    section("Invalid on/off value → error");
    CHECK(!configParses("server { listen 80; location / { root /x; autoindex maybe; } }"), "'maybe' rejected");
    CHECK(!configParses("server { listen 80; location / { root /x; autoindex true; } }"), "'true' rejected");
    section_end();
}

static void test_cfg_bad_redirect() {
    section("Invalid redirect code → error");
    CHECK(!configParses("server { listen 80; location / { return 200 /x; } }"), "code 200 rejected");
    CHECK(!configParses("server { listen 80; location / { return 500 /x; } }"), "code 500 rejected");
    section_end();
}

static void test_cfg_bad_error_page() {
    section("Invalid error_page → error");
    CHECK(!configParses("server { listen 80; error_page 200 /x.html; location / { root /x; } }"), "code 200 rejected");
    CHECK(!configParses("server { listen 80; error_page 404 noslash.html; location / { root /x; } }"), "path without / rejected");
    section_end();
}

// ---- Config files from disk -------------------------------------------------

static void test_cfg_file(const std::string& path, bool expectPass) {
    std::string name = path.substr(path.rfind('/') + 1);
    section(name + (expectPass ? " (should pass)" : " (should fail)"));
    bool passed = configFileParses(path);
    if (expectPass)
        CHECK(passed, "config accepted");
    else
        CHECK(!passed, "config rejected");
    section_end();
}

// =============================================================================
//  MAIN
// =============================================================================

int main() {
    Logger::setLevel(Logger::ERROR); // suppress INFO/DEBUG/WARN during tests

    // --- HTTP Parser ---------------------------------------------------------
    group("HTTP REQUEST PARSER");

    std::cout << "\n  " << DIM << "Basic parsing" << RST << "\n";
    test_simple_get();
    test_query_string();
    test_post_content_length();
    test_delete_method();

    std::cout << "\n  " << DIM << "Incremental feeding" << RST << "\n";
    test_byte_by_byte();
    test_split_at_request_line();
    test_split_at_header();
    test_split_header_body();

    std::cout << "\n  " << DIM << "Chunked transfer encoding" << RST << "\n";
    test_chunked_simple();
    test_chunked_extensions();
    test_chunked_byte_by_byte();
    test_chunked_trailers();

    std::cout << "\n  " << DIM << "Error handling" << RST << "\n";
    test_malformed_request_line();
    test_unsupported_method();
    test_invalid_method_char();
    test_bad_uri();
    test_bad_version();
    test_missing_host();
    test_http10_no_host();
    test_cl_and_te();
    test_duplicate_cl();
    test_duplicate_host();
    test_post_no_length();
    test_unsupported_te();
    test_invalid_headers();
    test_bad_chunk_size();
    test_missing_chunk_crlf();

    std::cout << "\n  " << DIM << "Edge cases" << RST << "\n";
    test_content_length_zero();
    test_duplicate_nonrestricted_header();
    test_after_complete();
    test_after_error();
    test_reset();
    test_case_insensitive_headers();
    test_trimmed_header_values();
    test_request_line_too_long();
    test_leading_crlf();
    test_negative_cl();
    test_nonnumeric_cl();
    test_large_body();

    // --- Config Parser -------------------------------------------------------
    group("CONFIG PARSER");

    std::cout << "\n  " << DIM << "Valid configurations" << RST << "\n";
    test_cfg_minimal();
    test_cfg_full_featured();
    test_cfg_virtual_hosts();
    test_cfg_inheritance();
    test_cfg_cmbs_zero();
    test_cfg_size_suffixes();
    test_cfg_comments_and_quotes();
    test_cfg_directive_aliases();

    std::cout << "\n  " << DIM << "Invalid configurations" << RST << "\n";
    test_cfg_empty();
    test_cfg_bad_port();
    test_cfg_no_location();
    test_cfg_bad_location_path();
    test_cfg_unknown_directive();
    test_cfg_missing_semicolon();
    test_cfg_missing_brace();
    test_cfg_bad_method();
    test_cfg_bad_size();
    test_cfg_nothing_to_serve();
    test_cfg_duplicate_location();
    test_cfg_nested_server();
    test_cfg_bad_on_off();
    test_cfg_bad_redirect();
    test_cfg_bad_error_page();

    // Config files from disk (if they exist)
    group("CONFIG FILES");
    const char* valid_files[] = {
        "configs/valid/minimal.conf", "configs/valid/default.conf",
        "configs/valid/typical.conf", "configs/valid/cgi.conf",
        "configs/valid/edge_cases.conf", "configs/valid/redirects.conf",
        "configs/valid/virtual_hosts.conf", "configs/valid/duplicate_server.conf",
        NULL
    };
    const char* invalid_files[] = {
        "configs/invalid/empty.conf", "configs/invalid/bad_port.conf",
        "configs/invalid/bad_method.conf", "configs/invalid/missing_brace.conf",
        "configs/invalid/missing_semicolon.conf", "configs/invalid/nested_server.conf",
        "configs/invalid/unknow_directive.conf",
        NULL
    };
    for (int i = 0; valid_files[i]; ++i) {
        std::ifstream f(valid_files[i]);
        if (f.good()) { f.close(); test_cfg_file(valid_files[i], true); }
    }
    for (int i = 0; invalid_files[i]; ++i) {
        std::ifstream f(invalid_files[i]);
        if (f.good()) { f.close(); test_cfg_file(invalid_files[i], false); }
    }

    // --- Summary -------------------------------------------------------------
    std::cout << "\n" << BLD << "━━━ SUMMARY ━━━" << RST << "\n\n";
    std::cout << "  " << GRN << "✓ " << g_total_pass << " passed" << RST << "\n";
    if (g_total_fail > 0)
        std::cout << "  " << RED << "✗ " << g_total_fail << " failed" << RST << "\n";
    else
        std::cout << "  " << DIM << "✗ 0 failed" << RST << "\n";
    std::cout << "\n";

    return g_total_fail == 0 ? 0 : 1;
}
