#include "http/RequestHandler.hpp"
#include "http/HttpStatus.hpp"

RequestHandler::RequestHandler() {}
RequestHandler::~RequestHandler() {}

// Stub: We will implement routing, method dispatch,
// static file serving, autoindex, redirects, uploads, and CGI dispatch later.
HttpResponse RequestHandler::handle(const HttpRequest& /*request*/,
                                    const ServerConfig& config) {
    return buildErrorResponse(501, config);
}

HttpResponse RequestHandler::handleGet(const HttpRequest& /*request*/,
                                       const ServerConfig& config,
                                       const LocationConfig& /*location*/) {
    return buildErrorResponse(501, config);
}

HttpResponse RequestHandler::handlePost(const HttpRequest& /*request*/,
                                        const ServerConfig& config,
                                        const LocationConfig& /*location*/) {
    return buildErrorResponse(501, config);
}

HttpResponse RequestHandler::handleDelete(const HttpRequest& /*request*/,
                                          const ServerConfig& config,
                                          const LocationConfig& /*location*/) {
    return buildErrorResponse(501, config);
}
// Stub

HttpResponse RequestHandler::buildErrorResponse(int code, const ServerConfig& /*config*/) {
    HttpResponse resp;
    resp.setStatus(code);
    resp.setHeader("Content-Type", "text/html; charset=utf-8");
    resp.setBody(HttpStatus::getDefaultErrorPage(code));
    return resp;
}

HttpResponse RequestHandler::buildRedirectResponse(const LocationConfig& location) {
    HttpResponse resp;
    int code = location.getRedirectCode();
    if (code == 0)
        code = 302;
    resp.setStatus(code);
    resp.setHeader("Location", location.getRedirect());
    resp.setBody("");
    return resp;
}
