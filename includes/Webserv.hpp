#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#include <string>

# define WEBSERV_VERSION "0.1.0"
# define DEFAULT_CONFIG_PATH "configs/valid/default.conf"

# define  DEFAULT_HOST "0.0.0.0"
# define  DEFAULT_PORT 8080
# define  DEFAULT_BACKLOG 128
# define DEFAULT_BUFFER_SIZE 42

# define DEFAULT_CLIENT_MAX_BODY_SIZE (1024 * 1024)
# define CLIENT_TIMEOUT_SECONDS 60
# define CGI_TIMEOUT_SECONDS 30

#endif
