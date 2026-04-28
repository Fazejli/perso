#ifndef CONFIG_HPP
# define CONFIG_HPP

# include <vector>
# include "config/ServerConfig.hpp"

// Top-level container holding every parsed `server { ... }` block. Produced
// by ConfigParser and consumed by Server.

class Config {
	public:
    	Config();
    	Config(const Config& other);
    	Config& operator=(const Config& other);
    	~Config();

		const std::vector<ServerConfig>& getServers() const;
		void 							addServer(const ServerConfig& server);
		bool							empty() const;
		size_t 							size() const;

	private:
		std::vector<ServerConfig> _servers;
};

#endif
