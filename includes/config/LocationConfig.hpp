#ifndef LOCATIONCONFIG_HPP
# define LOCATIONCONFIG_HPP

# include <string>
# include <vector>
#include <map>

// Holds the configuration for a single `location { ... }` block inside a
// server block. Owns no resources, fully copyable.

class LocationConfig {
	public :
		LocationConfig();
		LocationConfig(const LocationConfig& other);
		LocationConfig& operator=(const LocationConfig& other);
		~LocationConfig();

		const std::string& 							getPath() const;
		const std::string& 							getRoot() const;
		const std::string& 							getIndex() const;
		bool 										getAutoindex() const;
		const std::vector<std::string>&				getAllowedMethods() const;
		const std::string& 							getRedirect() const;
		int 										getRedirectCode() const;
		const std::string& 							getUploadStore() const;
		bool 										getUploadEnabled() const;
		const std::map<std::string, std::string>& 	getCgiHandlers() const;

		void setPath(const std::string& path);
		void setRoot(const std::string& root);
		void setIndex(const std::string& index);
		void setAutoindex(bool autoindex);
		void addAllowedMethods(const std::string& methods);
		void setAllowedMethods(const std::vector<std::string>& methods);
		void setRedirect(const std::string& target, int code);
		void setUploadStore(const std::string& uploadStore);
		void setUploadEnabled(bool enbaled);
		void addCgiHandler(const std::string& extension, const std::string& interpreter);

		bool 		isMethodAllowed(const std::string& method) const;
		bool 		hasCgiForExtension(const std::string&extension) const;
		std::string getCgiInterpreter(const std::string& extension) const;
		bool 		hasRedirect() const;

	private:
		std::string 						_path;
		std::string 						_root;
		std::string 						_index;
		bool 								_autoindex;
		std::vector<std::string>			_allowedMethods;
		std::string 						_redirect;
		int 								_redirectCode;
		std::string 						_uploadStore;
		bool 								_uploadEnabled;
		std::map<std::string, std::string> 	_cgiHandlers;
};

#endif
