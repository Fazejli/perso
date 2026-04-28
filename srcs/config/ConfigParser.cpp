#include "config/ConfigParser.hpp"
#include "utils/Logger.hpp"
#include <fstream>

ConfigParser::ParseError::ParseError(const std::string& msg)
	: std::runtime_error(msg) {}

ConfigParser::ConfigParser() {}
ConfigParser::~ConfigParser() {}

Config ConfigParser::parse(const std::string& filepath) {
	std::ifstream file(filepath.c_str()); // will be replaced
	if (!file.is_open()) {
		throw ParseError("Cannot open configuration file: " + filepath);
	}
	file.close();

	Logger::warn("ConfigParser is a stub: returning empty Config.");
	Logger::warn("Real parsing will be implemented later");

	return Config();
}
