#ifndef CGIHANDLER_HPP
# define CGIHANDLER_HPP

# include <string>
# include <map>
# include "http/HttpRequest.hpp"
# include "http/HttpResponse.hpp"


// Manages the execution of one CGI sub-process. The pipes it creates are
// exposed to Server which integrates them into the single poll() loop.

// The Server is expected to:
//   1. call start() with the parsed request and the script info
//   2. register getInputFd() in poll for write-readiness (we feed the body)
//   3. register getOutputFd() in poll for read-readiness (we read the result)
//   4. call writeToCgi() / readFromCgi() when poll signals readiness
//   5. call buildResponse() once isFinished() returns true

// NOTE: every method is currently a stub. We will implement
// fork/execve/pipe wiring and CGI environment construction later.

class CGIHandler {
public:
    CGIHandler();
    ~CGIHandler();

    bool start(const HttpRequest& request,
               const std::string& scriptPath,
               const std::string& interpreter);

    bool writeToCgi();
    bool readFromCgi();
    void onChildExited(int status);

    bool isFinished() const;

    HttpResponse buildResponse() const;

    int  getInputFd() const;   // pipe end we write the request body into
    int  getOutputFd() const;  // pipe end we read the CGI's stdout from
    int  getPid() const;

private:
    CGIHandler(const CGIHandler& other);
    CGIHandler& operator=(const CGIHandler& other);

    std::map<std::string, std::string>
    buildEnvironment(const HttpRequest& request,
                     const std::string& scriptPath) const;

    int          _inputFd;
    int          _outputFd;
    int          _pid;
    std::string  _inputBuffer;   // body bytes still to send to CGI stdin
    std::string  _outputBuffer;  // raw CGI stdout collected so far
    bool         _finished;
    int          _exitStatus;
};

#endif
