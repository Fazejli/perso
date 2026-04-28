#ifndef LOCATIONCONFIG_HPP
# define LOCATIONCONFIG_HPP

# include <string>
# include <vector>
# include <map>

// Holds the configuration for a single `location { ... }` block inside a
// server block. Owns no resources, fully copyable.
//
// Note on inheritable fields:
//   _clientMaxBodySize == (size_t)-1 => "not set, inherit from server"
//   _errorPages.empty()              => "not set, fall back to server table"
// (the parser fills these in when the location did not specify them)
class LocationConfig {
	public:
		LocationConfig();
		LocationConfig(const LocationConfig& other);
		LocationConfig& operator=(const LocationConfig& other);
		~LocationConfig();

		const std::string&                              getPath() const;
		const std::string&                              getRoot() const;
		const std::string&                              getIndex() const;
		bool                                            getAutoindex() const;
		const std::vector<std::string>&                 getAllowedMethods() const;
		const std::string&                              getRedirect() const;
		int                                             getRedirectCode() const;
		const std::string&                              getUploadStore() const;
		bool                                            getUploadEnabled() const;
		const std::map<std::string, std::string>&      getCgiHandlers() const;
		size_t                                          getClientMaxBodySize() const;
		const std::map<int, std::string>&              getErrorPages() const;

		void setPath(const std::string& path);
		void setRoot(const std::string& root);
		void setIndex(const std::string& index);
		void setAutoindex(bool autoindex);
		void addAllowedMethod(const std::string& method);
		void setAllowedMethods(const std::vector<std::string>& methods);
		void setRedirect(const std::string& target, int code);
		void setUploadStore(const std::string& uploadStore);
		void setUploadEnabled(bool enabled);
		void addCgiHandler(const std::string& extension, const std::string& interpreter);
		void setClientMaxBodySize(size_t size);
		void addErrorPage(int code, const std::string& path);

		bool        isMethodAllowed(const std::string& method) const;
		bool        hasCgiForExtension(const std::string& extension) const;
		std::string getCgiInterpreter(const std::string& extension) const;
		bool        hasRedirect() const;
		bool        hasClientMaxBodySize() const;
		std::string getErrorPagePath(int code) const;

	private:
		std::string                         _path;
		std::string                         _root;
		std::string                         _index;
		bool                                _autoindex;
		std::vector<std::string>            _allowedMethods;
		std::string                         _redirect;
		int                                 _redirectCode;
		std::string                         _uploadStore;
		bool                                _uploadEnabled;
		std::map<std::string, std::string>  _cgiHandlers;
		size_t                              _clientMaxBodySize; // (size_t)-1 => inherit
		std::map<int, std::string>          _errorPages;
};

#endif
