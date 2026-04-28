#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

# include <string>
# include <vector>
# include <map>
# include "config/LocationConfig.hpp"

// Represents one `server { ... }` block. A configuration file may declare
// any number of these. Each is independent and owns its own list of
// LocationConfigs.

class ServerConfig {
	public :
		ServerConfig();
		ServerConfig(const ServerConfig& other);
		ServerConfig& operator=(const ServerConfig& other);
		~ServerConfig();

		const std::string&					getHost() const;
		int									getPort() const;
		const std::vector<std::string>& 	getServerNames() const;
		size_t								getClientMaxBodySize() const;
		const std::map<int, std::string>& 	getErrorPages() const;
		const std::vector<LocationConfig>&	 getLocations() const;

		void setHost(const std::string& host);
		void setPort(int port);
		void addServerName(const std::string& name);
		void setClientMaxBodySize(size_t size);
		void addErrorPage(int code, const std::string& path);
		void addLocation(const LocationConfig& location);

		const LocationConfig* 	 findLocation(const std::string& uri) const;
		std::string 			getErrorPagePath(int code) const;

	private :
    	std::string                       _host;
    	int                               _port;
    	std::vector<std::string>          _serverNames;
    	size_t                            _clientMaxBodySize;
    	std::map<int, std::string>        _errorPages;
    	std::vector<LocationConfig>       _locations;
};


#endif
