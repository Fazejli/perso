#include "config/LocationConfig.hpp"

LocationConfig::LocationConfig()
    : _path("/"),
      _root(""),
      _index("index.html"),
      _autoindex(false),
      _allowedMethods(),
      _redirect(""),
      _redirectCode(0),
      _uploadStore(""),
      _uploadEnabled(false),
      _cgiHandlers() {
}

LocationConfig::LocationConfig(const LocationConfig& other)
    : _path(other._path),
      _root(other._root),
      _index(other._index),
      _autoindex(other._autoindex),
      _allowedMethods(other._allowedMethods),
      _redirect(other._redirect),
      _redirectCode(other._redirectCode),
      _uploadStore(other._uploadStore),
      _uploadEnabled(other._uploadEnabled),
      _cgiHandlers(other._cgiHandlers) {
}

LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
    if (this != &other) {
        _path           = other._path;
        _root           = other._root;
        _index          = other._index;
        _autoindex      = other._autoindex;
        _allowedMethods = other._allowedMethods;
        _redirect       = other._redirect;
        _redirectCode   = other._redirectCode;
        _uploadStore    = other._uploadStore;
        _uploadEnabled  = other._uploadEnabled;
        _cgiHandlers    = other._cgiHandlers;
    }
    return *this;
}

LocationConfig::~LocationConfig() {}

const std::string&                              LocationConfig::getPath() const           { return _path; }
const std::string&                              LocationConfig::getRoot() const           { return _root; }
const std::string&                              LocationConfig::getIndex() const          { return _index; }
bool                                            LocationConfig::getAutoindex() const      { return _autoindex; }
const std::vector<std::string>&                 LocationConfig::getAllowedMethods() const { return _allowedMethods; }
const std::string&                              LocationConfig::getRedirect() const       { return _redirect; }
int                                             LocationConfig::getRedirectCode() const   { return _redirectCode; }
const std::string&                              LocationConfig::getUploadStore() const    { return _uploadStore; }
bool                                            LocationConfig::getUploadEnabled() const  { return _uploadEnabled; }
const std::map<std::string, std::string>&      LocationConfig::getCgiHandlers() const     { return _cgiHandlers; }

void LocationConfig::setPath(const std::string& path)            { _path = path; }
void LocationConfig::setRoot(const std::string& root)            { _root = root; }
void LocationConfig::setIndex(const std::string& index)          { _index = index; }
void LocationConfig::setAutoindex(bool autoindex)                { _autoindex = autoindex; }
void LocationConfig::addAllowedMethods(const std::string& method) { _allowedMethods.push_back(method); }
void LocationConfig::setAllowedMethods(const std::vector<std::string>& methods) { _allowedMethods = methods; }
void LocationConfig::setRedirect(const std::string& target, int code) {
    _redirect     = target;
    _redirectCode = code;
}
void LocationConfig::setUploadStore(const std::string& uploadStore) { _uploadStore = uploadStore; }
void LocationConfig::setUploadEnabled(bool enabled)                 { _uploadEnabled = enabled; }
void LocationConfig::addCgiHandler(const std::string& extension, const std::string& interpreter) {
    _cgiHandlers[extension] = interpreter;
}

bool LocationConfig::isMethodAllowed(const std::string& method) const {
	if (_allowedMethods.empty())
		return false;
	for (std::vector<std::string>::const_iterator it = _allowedMethods.begin();
		it != _allowedMethods.end(); ++it) {
		if (*it == method)
			return true;
	}
	return false;
}

bool LocationConfig::hasCgiForExtension(const std::string& extension) const {
	return _cgiHandlers.find(extension) != _cgiHandlers.end();
}

std::string LocationConfig::getCgiInterpreter(const std::string& extension) const {
	std::map<std::string, std::string>::const_iterator it = _cgiHandlers.find(extension);
	if (it == _cgiHandlers.end())
		return "";
	return it->second;
}

bool LocationConfig::hasRedirect() const {
	return !_redirect.empty();
}
