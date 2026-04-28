#ifndef HTTPSTATUS_HPP
# define HTTPSTATUS_HPP

# include <string>

// Stateless utilities to map HTTP status codes to their reason phrases and
// to default error pages.

class HttpStatus {
	public :
		static std::string getReasonPhrase(int code);
		static std::string getDefaultErrorPage(int code);
		static bool isError(int code);

	private :
		HttpStatus();
		HttpStatus(const HttpStatus& other);
		HttpStatus& operator=(const HttpStatus& other);
		~HttpStatus();
};

#endif
