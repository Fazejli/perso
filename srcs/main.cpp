#include <iostream>
#include <stdexcept>
#include <string>
#include <csignal>

#include "Webserv.hpp"
#include "config/Config.hpp"
#include "config/ConfigParser.hpp"
#include "server/Server.hpp"
#include "utils/Logger.hpp"

namespace {

	void printUsage(const char* prog) {
	std::cerr << "Usage: " << prog << " [configuration file]" << std::endl;
	std::cerr << " If no configuration file is provided, "<< DEFAULT_CONFIG_PATH << " will be used." << std::endl;
	}
}

int main(int ac, char **av) {
	std::string configPath = DEFAULT_CONFIG_PATH;

	if (ac > 2) {
		printUsage(av[0]);
		return 1;
	}
	if (ac == 2) {
		std::string arg(av[1]);
		if (arg == "-h" || arg== "-help") {
			printUsage(av[0]);
			return 0;
		}
		configPath = arg;
	}

	std::signal(SIGPIPE, SIG_IGN);
	try {
		Logger::info(std::string("Webserv ") + WEBSERV_VERSION + " starting...");
		Logger::info("Loading configuration from: " + configPath);
		ConfigParser parser;
		Config config = parser.parse(configPath);
		Server server(config);
		server.run();
	}
	catch (const std::exception& e) {
		Logger::error(std::string("Fatal: ") + e.what());
		return 1;
	}

	Logger::info("Webserv shutting down clearly");
	return 0;
}
