#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

# include <string>
# include <map>

// Builder/serializer for an HTTP/1.1 response. Headers are stored in a map
// (case-insensitive comparison should be applied at insertion time).
// serialize() produces the wire-format string ready to be written to a
// socket.

class HttpResponse {
	public:
    	HttpResponse();
    	HttpResponse(const HttpResponse& other);
    	HttpResponse& operator=(const HttpResponse& other);
    	~HttpResponse();

		void setStatus(int code);
		void setStatus(int code, const std::string& message);
		void setHeader(const std::string& name, const std::string& value);
		void setBody(const std::string& body);
		void appendBody(const std::string& data);
		void clear();

		int 										getStatusCode() const;
		const std::string& 							getStatusMessage() const;
		const std::map<std::string, std::string>& 	getHeaders() const;
		const std::string& 							getBody() const;

		std::string serialize() const;

	private :
		int 								_statusCode;
		std::string 						_statusMessage;
		std::map<std::string, std::string>  _headers;
		std::string 						_body;
};

#endif
