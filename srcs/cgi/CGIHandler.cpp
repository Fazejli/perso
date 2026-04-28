#include "cgi/CGIHandler.hpp"
#include "utils/Logger.hpp"

CGIHandler::CGIHandler()
    : _inputFd(-1),
      _outputFd(-1),
      _pid(-1),
      _inputBuffer(),
      _outputBuffer(),
      _finished(false),
      _exitStatus(0) {
}

// Later: close pipe fds and reap the child if still alive.
CGIHandler::~CGIHandler() {
}

bool CGIHandler::start(const HttpRequest& /*request*/,
                       const std::string& /*scriptPath*/,
                       const std::string& /*interpreter*/) {
    Logger::warn("CGIHandler::start is a stub.");
    return false;
}

bool CGIHandler::writeToCgi() {
    return false;
}

bool CGIHandler::readFromCgi() {
    return false;
}

void CGIHandler::onChildExited(int status) {
    _exitStatus = status;
    _finished   = true;
}

bool CGIHandler::isFinished() const { return _finished; }

HttpResponse CGIHandler::buildResponse() const {
    HttpResponse resp;
    resp.setStatus(501);
    resp.setHeader("Content-Type", "text/plain");
    resp.setBody("CGI not yet implemented");
    return resp;
}

int CGIHandler::getInputFd() const  { return _inputFd; }
int CGIHandler::getOutputFd() const { return _outputFd; }
int CGIHandler::getPid() const      { return _pid; }

// We will fill: REQUEST_METHOD, CONTENT_LENGTH, CONTENT_TYPE,
// QUERY_STRING, SCRIPT_NAME, SCRIPT_FILENAME, PATH_INFO, SERVER_NAME,
// SERVER_PORT, SERVER_PROTOCOL, GATEWAY_INTERFACE, REDIRECT_STATUS,
// and every HTTP_* header later.
std::map<std::string, std::string>
CGIHandler::buildEnvironment(const HttpRequest& /*request*/,
                             const std::string& /*scriptPath*/) const {
    return std::map<std::string, std::string>();
}
