#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

# include <string>
# include <map>


// Incremental HTTP/1.1 request parser. Data is fed to it via appendData()
// as it arrives from recv(); the parser advances through its state machine
// and signals completion when the full request has been received.

// State machine:
//   PARSE_REQUEST_LINE -> waiting for "METHOD URI VERSION\r\n"
//   PARSE_HEADERS      -> waiting for header lines, terminated by \r\n\r\n
//   PARSE_BODY         -> waiting for Content-Length bytes (or chunked end)
//   PARSE_COMPLETE     -> done, request can be handled
//   PARSE_ERROR        -> malformed request, server should reply 400

class HttpRequest {
	public:
    	enum ParseState {
    	    PARSE_REQUEST_LINE,
    	    PARSE_HEADERS,
    	    PARSE_BODY,
    	    PARSE_COMPLETE,
    	    PARSE_ERROR
    	};

    	HttpRequest();
    	HttpRequest(const HttpRequest& other);
    	HttpRequest& operator=(const HttpRequest& other);
    	~HttpRequest();

		bool appendData(const std::string& data);
		void reset();

		ParseState                                getState() const;
    	bool                                      isComplete() const;
    	bool                                      hasError() const;
    	int                                       getErrorCode() const;

    	const std::string&                        getMethod() const;
    	const std::string&                        getUri() const;
    	const std::string&                        getPath() const;          // URI minus query string
    	const std::string&                        getQueryString() const;
    	const std::string&                        getVersion() const;
    	const std::map<std::string, std::string>& getHeaders() const;
    	std::string                               getHeader(const std::string& name) const;
    	bool                                      hasHeader(const std::string& name) const;
    	const std::string&                        getBody() const;

	private:
		bool parseRequestLine();
   		bool parseHeaders();
    	bool finalizeHeaders();
    	bool parseBody();
    	bool parseContentLengthBody();
    	bool parseChunkedBody();

    	ParseState                          _state;
    	int                                 _errorCode;
   		std::string                         _buffer;        // unparsed bytes
    	std::string                         _method;
    	std::string                         _uri;
    	std::string                         _path;
    	std::string                         _queryString;
    	std::string                         _version;
    	std::map<std::string, std::string>  _headers;       // header names lowercased
    	std::string                         _body;
    	size_t                              _expectedBodySize;
    	bool                                _chunked;
};

#endif
