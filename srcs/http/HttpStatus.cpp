#include "http/HttpStatus.hpp"
#include "utils/StringUtils.hpp"
#include <sstream>

std::string HttpStatus::getReasonPhrase(int code) {
    switch (code) {
        // 1xx
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        // 2xx
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        // 3xx
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
        // 4xx
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 411: return "Length Required";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        // 5xx
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default:  return "Unknown";
    }
}

std::string HttpStatus::getDefaultErrorPage(int code) {
    std::ostringstream oss;
    std::string reason = getReasonPhrase(code);

    oss << "<!DOCTYPE html>\n"
        << "<html>\n"
        << "<head>\n"
        << "  <meta charset=\"UTF-8\">\n"
        << "  <title>" << code << " " << reason << "</title>\n"
        << "  <style>\n"
        << "    body { font-family: sans-serif; text-align: center; padding: 50px; }\n"
        << "    h1 { color: #444; }\n"
        << "    hr { border: none; border-top: 1px solid #ddd; }\n"
        << "  </style>\n"
        << "</head>\n"
        << "<body>\n"
        << "  <h1>" << code << " " << reason << "</h1>\n"
        << "  <hr>\n"
        << "  <p>webserv</p>\n"
        << "</body>\n"
        << "</html>\n";
    return oss.str();
}

bool HttpStatus::isError(int code) {
    return code >= 400;
}
