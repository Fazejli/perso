#include "http/HttpRequest.hpp"
#include "utils/StringUtils.hpp"
#include "Webserv.hpp"
#include <cstdlib>
#include <cctype>

namespace {

	const size_t MAX_REQUEST_LINE_SIZE 	= 8192;
	const size_t MAX_HEADER_SIZE		= 8192;
	const size_t MAX_HEADERS_TOTAL		= 65536;
	const size_t MAX_HEADERS_COUNT		= 100;

	bool parseHex(const std::string& s, size_t& out) {
		if (s.empty()) return false;
		out = 0;
		for (std::string::size_type i = 0; i < s.size(); i++) {
			char c = s[i];
			size_t digit;
			if (c >= '0' && c <= '9')		digit = static_cast<size_t>(c - '0');
			else if (c >= 'a' && c <= 'f')	digit = static_cast<size_t>(c - 'a' + 10);
			else if (c >= 'A' && c <= 'F')	digit = static_cast<size_t>(c - 'A' + 10);
			else							return false;
			if (out > (static_cast<size_t>(-1) - digit) / 16)
				return false;
			out = out * 16 + digit;
		}
		return true;
	}

	bool isTokenChar(unsigned char c) {
		if (c <= 0x20 || c >= 0x7F) return false;
		switch (c) {
            case '(': case ')': case ',': case '/': case ':': case ';':
            case '<': case '=': case '>': case '?': case '@': case '[':
            case '\\': case ']': case '{': case '}': case '"':
				return false;
			default:
				return true;
		}
	}

	bool isValidToken(const std::string& s) {
		if (s.empty()) return false;
		for (std::string::size_type i = 0; i < s.size(); i++) {
			if (!isTokenChar(static_cast<unsigned char>(s[i])))
				return false;
		}
		return true;
	}

	bool isValidUriChar(unsigned char c) {
		return c > 0x20 && c < 0x7F;
	}

	bool isValidUri(const std::string& s) {
        if (s.empty()) return false;
        for (std::string::size_type i = 0; i < s.size(); ++i) {
            if (!isValidUriChar(static_cast<unsigned char>(s[i])))
                return false;
        }
        return true;
    }

	void setError(HttpRequest::ParseState& state, int& errCode, int code) {
		state = HttpRequest::PARSE_ERROR;
		errCode = code;
	}
}

HttpRequest::HttpRequest()
    : _state(PARSE_REQUEST_LINE),
      _errorCode(0),
      _buffer(),
      _method(),
      _uri(),
      _path(),
      _queryString(),
      _version(),
      _headers(),
      _body(),
      _expectedBodySize(0),
      _chunked(false) {
}

HttpRequest::HttpRequest(const HttpRequest& other)
    : _state(other._state),
      _errorCode(other._errorCode),
      _buffer(other._buffer),
      _method(other._method),
      _uri(other._uri),
      _path(other._path),
      _queryString(other._queryString),
      _version(other._version),
      _headers(other._headers),
      _body(other._body),
      _expectedBodySize(other._expectedBodySize),
      _chunked(other._chunked) {
}

HttpRequest& HttpRequest::operator=(const HttpRequest& other) {
    if (this != &other) {
        _state            = other._state;
        _errorCode        = other._errorCode;
        _buffer           = other._buffer;
        _method           = other._method;
        _uri              = other._uri;
        _path             = other._path;
        _queryString      = other._queryString;
        _version          = other._version;
        _headers          = other._headers;
        _body             = other._body;
        _expectedBodySize = other._expectedBodySize;
        _chunked          = other._chunked;
    }
    return *this;
}

HttpRequest::~HttpRequest() {}

void HttpRequest::reset() {
    _state            = PARSE_REQUEST_LINE;
    _errorCode        = 0;
    _buffer.clear();
    _method.clear();
    _uri.clear();
    _path.clear();
    _queryString.clear();
    _version.clear();
    _headers.clear();
    _body.clear();
    _expectedBodySize = 0;
    _chunked          = false;
}

HttpRequest::ParseState HttpRequest::getState() const   { return _state; }
bool                    HttpRequest::isComplete() const { return _state == PARSE_COMPLETE; }
bool                    HttpRequest::hasError() const   { return _state == PARSE_ERROR; }
int                     HttpRequest::getErrorCode() const { return _errorCode; }

const std::string&                        HttpRequest::getMethod() const      { return _method; }
const std::string&                        HttpRequest::getUri() const         { return _uri; }
const std::string&                        HttpRequest::getPath() const        { return _path; }
const std::string&                        HttpRequest::getQueryString() const { return _queryString; }
const std::string&                        HttpRequest::getVersion() const     { return _version; }
const std::map<std::string, std::string>& HttpRequest::getHeaders() const     { return _headers; }
const std::string&                        HttpRequest::getBody() const        { return _body; }

std::string HttpRequest::getHeader(const std::string& name) const {
	std::map<std::string, std::string>::const_iterator it = _headers.find(StringUtils::toLower(name));
	if (it == _headers.end())
		return "";
	return it->second;
}

bool HttpRequest::hasHeader(const std::string& name) const {
	return _headers.find(StringUtils::toLower(name)) != _headers.end();
}

bool HttpRequest::appendData(const std::string& data) {
	if (_state == PARSE_COMPLETE || _state == PARSE_ERROR)
		return _state == PARSE_COMPLETE;

	_buffer += data;

	bool progress = true;
	while (progress && _state != PARSE_COMPLETE && _state != PARSE_ERROR) {
		switch (_state) {
			case PARSE_REQUEST_LINE:
				progress = parseRequestLine();
				break;
			case PARSE_HEADERS:
				progress = parseHeaders();
				break;
			case PARSE_BODY:
    			progress = parseBody();
   				break;
			default:
				progress = false;
				break;
		}
	}
	return _state == PARSE_COMPLETE;
}

// Looks for the first CRLF in the buffer. If none, we wait for more data
// (unless the buffer has grown past MAX_REQUEST_LINE_SIZE, in which case
// we bail out with 414 URI Too Long).

// Valid input: "METHOD SP URI SP HTTP-VERSION CRLF"
bool HttpRequest::parseRequestLine() {
	std::string::size_type crlf = _buffer.find("\r\n");
	if (crlf == std::string::npos) {
		if (_buffer.size() > MAX_REQUEST_LINE_SIZE) {
			setError(_state, _errorCode, 414);
			return false;
		}
		return false;
	}

	if (crlf > MAX_REQUEST_LINE_SIZE) {
		setError(_state, _errorCode, 414);
		return false;
	}

	std::string line = _buffer.substr(0, crlf);
	_buffer.erase(0, crlf + 2);
	if (line.empty()) {
		setError(_state, _errorCode, 400);
		return false;
	}

	std::string::size_type sp1 = line.find(' ');
	if (sp1 == std::string::npos) {
		setError(_state, _errorCode, 400);
		return false;
	}
	std::string::size_type sp2 = line.find(' ', sp1 + 1);
	if (sp2 == std::string::npos) {
		setError(_state, _errorCode, 400);
		return false;
	}
	if (line.find(' ', sp2 + 1) != std::string::npos) {
		setError(_state, _errorCode, 400);
		return false;
	}
	_method = line.substr(0, sp1);
	_uri = line.substr(sp1 + 1, sp2 - sp1 - 1);
	_version = line.substr(sp2 + 1);

	if (!isValidToken(_method)) {
		setError(_state, _errorCode, 400);
		return false;
	}
	if (_method != "GET" && _method != "POST" && _method != "DELETE") {
		setError(_state, _errorCode, 501);
		return false;
	}

	if (_uri.empty() || !isValidUri(_uri)) {
		setError(_state, _errorCode, 400);
		return false;
	}
	if (_uri.size() > MAX_REQUEST_LINE_SIZE) {
		setError(_state, _errorCode, 414);
		return false;
	}
	if (_uri[0] != '/') {
		setError(_state, _errorCode, 400);
		return false;
	}

	std::string::size_type q = _uri.find('?');
	if (q == std::string::npos) {
		_path = _uri;
		_queryString = "";
	} else {
		_path = _uri.substr(0, q);
		_queryString = _uri.substr(q + 1);
	}

	if (_version != "HTTP/1.1" && _version != "HTTP/1.0") {
		if (_version.size() >= 5 && _version.compare(0, 5, "HTTP/") == 0) {
			setError(_state, _errorCode, 505);
		} else {
			setError(_state, _errorCode, 400);
		}
		return false;
	}

	_state = PARSE_HEADERS;
	return true;
}

// Reads lines until an empty line (CRLF by itself) is found. Each line is
// "Name: value". Header names are lowercased for storage so lookups are
// case-insensitive.

// On end-of-headers we inspect Content-Length / Transfer-Encoding and
// decide whether a body follows.
bool HttpRequest::parseHeaders() {
	size_t totalHeaderBytes = 0;
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		it != _headers.end(); ++it) {
		totalHeaderBytes += it->first.size() + it->second.size() + 4;
	}

	while (true) {
		std::string::size_type crlf = _buffer.find("\r\n");
		if (crlf == std::string::npos) {
			if (_buffer.size() > MAX_HEADER_SIZE) {
            	setError(_state, _errorCode, 400);
            	return false;
			}
			return false;
		}
		if (crlf == 0) {
			_buffer.erase(0, 2);
			return finalizeHeaders();
		}
		if (crlf > MAX_HEADER_SIZE) {
			setError(_state, _errorCode, 400);
			return false;
		}
		std::string line = _buffer.substr(0, crlf);
		_buffer.erase(0, crlf + 2);
		if (line[0] == ' ' || line[0] == '\t') {
			setError(_state, _errorCode, 400);
			return false;
		}

		std::string::size_type colon = line.find(':');
		if (colon == std::string::npos || colon == 0) {
			setError(_state, _errorCode, 400);
			return false;
		}
		std::string name = line.substr(0, colon);
		std::string value = line.substr(colon + 1);
		if (!isValidToken(name)) {
			setError(_state, _errorCode, 400);
			return false;
		}

		value = StringUtils::trim(value);
		name = StringUtils::toLower(name);
		totalHeaderBytes += name.size() + value.size() + 4;
		if (totalHeaderBytes > MAX_HEADERS_TOTAL ||
			_headers.size() >= MAX_HEADERS_COUNT) {
			setError(_state, _errorCode, 431);
			return false;
		}

		std::map<std::string, std::string>::iterator existing = _headers.find(name);
		if (existing != _headers.end()) {
			if (name == "content-length" || name == "host") {
				setError(_state, _errorCode, 400);
				return false;
			}
			existing->second += ", ";
			existing->second += value;
		} else {
			_headers[name] = value;
		}
	}
}

// Called once the empty line closing the header block has been consumed.
// Decides whether a body follows, and if so in which format.
bool HttpRequest::finalizeHeaders() {
	if (_version == "HTTP/1.1" && !hasHeader("host")) { // HTTP/1.0 does not require host
		setError(_state, _errorCode, 400);
		return false;
	}

	bool hasCL = hasHeader("content-length");
	bool hasTE = hasHeader("transfer-encoding");
	if (hasCL && hasTE) {
		setError(_state, _errorCode, 400);
		return false;
	}

	if (hasTE) {
		std::string te = StringUtils::toLower(getHeader("transfer-encoding"));
		if (te != "chunked") {
			setError(_state, _errorCode, 501);
			return false;
		}
		_chunked = true;
		_expectedBodySize = 0;
		_state = PARSE_BODY;
		return true;
	}

	if (hasCL) {
		size_t len;
		if (!StringUtils::toSize(getHeader("content-length"), len)) {
			setError(_state, _errorCode, 400);
			return false;
		}
		_expectedBodySize = len;
		_chunked = false;
		if (len == 0) {
			_state = PARSE_COMPLETE;
			return true;
		}
		_state = PARSE_BODY;
		return true;
	}

	if (_method == "POST") {
		setError(_state, _errorCode, 411);
		return false;
	}
	_state = PARSE_COMPLETE;
	return true;
}

bool HttpRequest::parseBody() {
	if (_chunked)
		return parseChunkedBody();
	return parseContentLengthBody();
}

bool HttpRequest::parseContentLengthBody() {
	size_t remaining = _expectedBodySize - _body.size();
	if (_buffer.size() < remaining) {
		_body.append(_buffer);
		_buffer.clear();
		return false;
	}
	_body.append(_buffer, 0, remaining);
	_buffer.erase(0, remaining);
	_state = PARSE_COMPLETE;
	return true;
}

// Wire format:
//     <hex-size> [;ext] CRLF
//     <size bytes of data> CRLF
//     ...
//     0 CRLF
//     [trailer-headers...]
//     CRLF
//
// We overload _expectedBodySize as a tiny bit of FSM state:
//   AWAITING_SIZE_LINE (0)  -> next step is to parse the chunk-size line
//   > 0 and < max           -> that many bytes of the current chunk remain
//   READING_TRAILER (max)   -> we've seen the 0-chunk, skipping trailers
bool HttpRequest::parseChunkedBody() {
	static const size_t AWAITING_SIZE_LINE = 0;
	static const size_t READING_TRAILER = static_cast<size_t>(-1);

	while (true) {
		if (_expectedBodySize == READING_TRAILER) {
			std::string::size_type crlf = _buffer.find("\r\n");
			if (crlf == std::string::npos) {
				if (_buffer.size() > MAX_HEADER_SIZE) {
					setError(_state, _errorCode, 400);
					return false;
				}
				return false;
			}
			if (crlf == 0) {
				_buffer.erase(0, 2);
				_state = PARSE_COMPLETE;
				return true;
			}
			_buffer.erase(0, crlf + 2);
			continue;
		}

		if (_expectedBodySize == AWAITING_SIZE_LINE) {
			std::string::size_type crlf = _buffer.find("\r\n");
			if (crlf == std::string::npos) {
				if (_buffer.size() > MAX_HEADER_SIZE) {
					setError(_state, _errorCode, 400);
					return false;
				}
				return false;
			}
			std::string sizeLine = _buffer.substr(0, crlf);
			_buffer.erase(0, crlf + 2);
			std::string::size_type semi = sizeLine.find(';');
			std::string hexPart = (semi == std::string::npos)
								   ? sizeLine
								   : sizeLine.substr(0, semi);
			hexPart = StringUtils::trim(hexPart);

			size_t chunkSize;
			if (!parseHex(hexPart, chunkSize)) {
				setError(_state, _errorCode, 400);
				return false;
			}
			if (chunkSize == 0) {
				_expectedBodySize = READING_TRAILER;
				continue;
			}
			if (_body.size() + chunkSize > DEFAULT_CLIENT_MAX_BODY_SIZE * 16) {
				setError(_state, _errorCode, 413);
				return false;
			}
			_expectedBodySize = chunkSize;
			continue;
		}

		if (_buffer.size() < _expectedBodySize + 2) {
			return false;
		}
		_body.append(_buffer, 0, _expectedBodySize);
		if (_buffer[_expectedBodySize] != '\r' || _buffer[_expectedBodySize + 1] != '\n') {
			setError(_state, _errorCode, 400);
			return false;
		}
		_buffer.erase(0, _expectedBodySize + 2);
		_expectedBodySize = AWAITING_SIZE_LINE;
	}
}
