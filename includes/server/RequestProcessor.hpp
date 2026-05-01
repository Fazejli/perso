#ifndef REQUESTPROCESSOR_HPP
# define REQUESTPROCESSOR_HPP

# include "server/Client.hpp"
# include "server/Server.hpp"
# include "config/Config.hpp"
# include "http/HttpRequest.hpp"
# include "http/HttpResponse.hpp"
# include "http/RequestHandler.hpp"

// RequestProcessor
//
// Sits between the network layer (Server) and the HTTP layer (RequestHandler).
// Its single responsibility: given a Client whose read buffer has grown,
// feed bytes into its HttpRequest, detect completion or error, dispatch to
// RequestHandler, and put the serialised response into the write buffer so
// Server can flush it.
//
// This keeps Server free of HTTP logic and RequestHandler free of socket/
// buffer concerns.

class RequestProcessor {
public:
    RequestProcessor();
    ~RequestProcessor();

    // Feed bytes from client._readBuffer into client._request.
    // Returns true when a full request has been received (or an error
    // has been detected) and the response is ready in _writeBuffer.
    bool process(Client& client, const Config& config);

private:
    RequestProcessor(const RequestProcessor&);
    RequestProcessor& operator=(const RequestProcessor&);

    // Pick the best ServerConfig for this request (Host header matching).
    const ServerConfig* selectServer(const HttpRequest& request,
                                     const Config&      config,
                                     const ServerConfig* listenServer) const;

    // Build a ready-to-send error response and put it in the write buffer.
    void sendError(Client& client, int code, const ServerConfig* srv);

    RequestHandler _handler;
};

#endif
