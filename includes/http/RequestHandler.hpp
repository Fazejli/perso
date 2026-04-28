#ifndef REQUESTHANDLER_HPP
# define REQUESTHANDLER_HPP

# include "http/HttpRequest.hpp"
# include "http/HttpResponse.hpp"
# include "config/ServerConfig.hpp"

// Pure logic: takes a parsed HttpRequest and the ServerConfig that should
// answer it, and returns the HttpResponse to send. Knows nothing about
// sockets or polling. This separation makes the dispatcher trivially
// testable in isolation from the network layer.

// NOTE: handle() currently returns a hard-coded "501 Not
// Implemented". The real routing will come later.

class RequestHandler {
	public:
   		RequestHandler();
    	~RequestHandler();

		HttpResponse handle(const HttpRequest& request, const ServerConfig& config);
	private :
		    RequestHandler(const RequestHandler& other);
			RequestHandler& operator=(const RequestHandler& other);

			HttpResponse handleGet(const HttpRequest& request,
                           const ServerConfig& config,
                           const LocationConfig& location);
    		HttpResponse handlePost(const HttpRequest& request,
                            const ServerConfig& config,
                            const LocationConfig& location);
    		HttpResponse handleDelete(const HttpRequest& request,
                              const ServerConfig& config,
                              const LocationConfig& location);

   			HttpResponse buildErrorResponse(int code, const ServerConfig& config);
    		HttpResponse buildRedirectResponse(const LocationConfig& location);
};

#endif
