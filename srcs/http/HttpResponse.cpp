#include "http/HttpResponse.hpp"
#include "http/HttpStatus.hpp"
#include "utils/StringUtils.hpp"
#include <sstream>

HttpResponse::HttpResponse()
    : _statusCode(200),
      _statusMessage("OK"),
      _headers(),
      _body() {
}

HttpResponse::HttpResponse(const HttpResponse& other)
    : _statusCode(other._statusCode),
      _statusMessage(other._statusMessage),
      _headers(other._headers),
      _body(other._body) {
}

HttpResponse& HttpResponse::operator=(const HttpResponse& other) {
    if (this != &other) {
        _statusCode    = other._statusCode;
        _statusMessage = other._statusMessage;
        _headers       = other._headers;
        _body          = other._body;
    }
    return *this;
}

HttpResponse::~HttpResponse() {}

void HttpResponse::setStatus(int code) {
	_statusCode = code;
	_statusMessage = HttpStatus::getReasonPhrase(code);
}

void HttpResponse::setStatus(int code, const std::string& message) {
	_statusCode = code;
	_statusMessage = message;
}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
	_headers[name] = value;
}

void HttpResponse::setBody(const std::string& body) {
	_body = body;
}

void HttpResponse::appendBody(const std::string& data) {
	_body += data;
}

void HttpResponse::clear() {
	_statusCode = 200;
	_statusMessage = "OK";
	_headers.clear();
	_body.clear();
}

int                                          HttpResponse::getStatusCode() const    { return _statusCode; }
const std::string&                           HttpResponse::getStatusMessage() const { return _statusMessage; }
const std::map<std::string, std::string>&    HttpResponse::getHeaders() const       { return _headers; }
const std::string&                           HttpResponse::getBody() const          { return _body; }

std::string HttpResponse::serialize() const {
	std::ostringstream oss;

	oss << "HTTP/1.1 " << _statusCode<< " " << _statusMessage << "\r\n";

	bool hasContentLength = _headers.find("Content-Lenght") != _headers.end();
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		it != _headers.end(); ++it) {
			oss << it->first << ": " << it->second<< "\r\n";
	}
	if (!hasContentLength) {
		oss << "Content-Lenght: " << StringUtils::toString(_body.size()) << "\r\n";
	}

	oss << "\r\n";
	oss << _body;
	return oss.str();
}

