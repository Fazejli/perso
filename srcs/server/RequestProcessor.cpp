#include "server/RequestProcessor.hpp"
#include "http/HttpStatus.hpp"
#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"

RequestProcessor::RequestProcessor() : _handler() {}
RequestProcessor::~RequestProcessor() {}

// ─────────────────────────────────────────────────────────────────────────────
// process()
//
// Called by Server every time new bytes have been appended to client's
// _readBuffer.  We drain _readBuffer into the HttpRequest parser, then:
//   • error  → serialise an error response, switch to WRITING
//   • complete → pick ServerConfig, dispatch, serialise, switch to WRITING
//   • incomplete → do nothing, wait for more data
//
// Returns true when the client is ready to write (state == WRITING).
// ─────────────────────────────────────────────────────────────────────────────
bool RequestProcessor::process(Client& client, const Config& config) {
    HttpRequest& req    = client.getRequest();
    std::string& rbuf   = client.getReadBuffer();

    // Feed buffered bytes into the HTTP parser.
    if (!rbuf.empty()) {
        req.appendData(rbuf);
        rbuf.clear();
    }

    // ── Parser raised an error ────────────────────────────────────────────
    if (req.hasError()) {
        int code = req.getErrorCode();
        if (code == 0) code = 400;
        Logger::warn("HTTP parse error " + StringUtils::toString(code)
                     + " from client fd=" + StringUtils::toString(client.getFd()));
        sendError(client, code, client.getServerConfig());
        return true;
    }

    // ── Still waiting for more data ───────────────────────────────────────
    if (!req.isComplete())
        return false;

    // ── Full request received ─────────────────────────────────────────────
    Logger::info("Request complete: " + req.getMethod() + " " + req.getUri()
                 + " from fd=" + StringUtils::toString(client.getFd()));

    // Virtual-host selection: we now know the Host header.
    const ServerConfig* srv = selectServer(req, config, client.getServerConfig());
    client.setServerConfig(srv);

    if (!srv) {
        sendError(client, 500, NULL);
        return true;
    }

    // Delegate to RequestHandler (static files, redirects, CGI, uploads …).
    HttpResponse resp = _handler.handle(req, *srv);

    // Serialise into the write buffer.
    client.getWriteBuffer() = resp.serialize();
    client.setState(Client::WRITING);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// selectServer()
//
// Among all ServerConfigs that share the same listening port as the socket
// that accepted this client, pick the one whose server_name matches the
// Host header (first match wins).  Fall back to the first server on that
// port (nginx-like default).
// ─────────────────────────────────────────────────────────────────────────────
const ServerConfig* RequestProcessor::selectServer(const HttpRequest&  request,
                                                    const Config&       config,
                                                    const ServerConfig* listenServer) const {
    // Extract hostname from Host header (strip optional :port).
    std::string host = request.getHeader("host");
    std::string::size_type colon = host.rfind(':');
    if (colon != std::string::npos)
        host = host.substr(0, colon);

    const std::vector<ServerConfig>& servers = config.getServers();

    // Determine the port we are listening on.
    int listenPort = listenServer ? listenServer->getPort() : 0;

    const ServerConfig* defaultSrv = listenServer; // fallback

    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfig& s = servers[i];

        // Only consider servers on the same port.
        if (s.getPort() != listenPort)
            continue;

        // Keep the first matching port as the default.
        if (defaultSrv == NULL)
            defaultSrv = &s;

        // Match server_name against Host header.
        if (host.empty())
            continue;
        const std::vector<std::string>& names = s.getServerNames();
        for (size_t j = 0; j < names.size(); ++j) {
            if (names[j] == host) {
                Logger::debug("Virtual host matched: " + host
                              + " → server on port "
                              + StringUtils::toString(s.getPort()));
                return &s;
            }
        }
    }

    return defaultSrv;
}

// ─────────────────────────────────────────────────────────────────────────────
// sendError()  — quick path for protocol-level errors
// ─────────────────────────────────────────────────────────────────────────────
void RequestProcessor::sendError(Client& client, int code, const ServerConfig* /*srv*/) {
    HttpResponse resp;
    resp.setStatus(code);
    resp.setHeader("Content-Type", "text/html; charset=utf-8");
    resp.setBody(HttpStatus::getDefaultErrorPage(code));
    client.getWriteBuffer() = resp.serialize();
    client.setState(Client::WRITING);
}
