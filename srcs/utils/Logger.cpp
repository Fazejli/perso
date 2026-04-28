#include "utils/Logger.hpp"
#include <iostream>
#include <ctime>

Logger::Level Logger::_minLevel = Logger::INFO;
bool		  Logger::_colorEnabled = true;

void Logger::setLevel(Level level) { _minLevel = level; }
void Logger::setColorEnabled(bool enabled) { _colorEnabled = enabled; }

const char* Logger::levelToString(Level level) {
	switch (level) {
		case DEBUG: return "DEBUG";
		case INFO: return "INFO ";
		case WARN: return "WARN ";
		case ERROR: return "ERROR";
	}
	return "?????";
}

const char* Logger::levelToColor(Level level) {
    switch (level) {
        case DEBUG: return "\033[0;36m";
        case INFO:  return "\033[0;32m";
        case WARN:  return "\033[0;33m";
        case ERROR: return "\033[0;31m";
    }
    return "";
}

void Logger::log(Level level, const std::string& message) {
	if (level < _minLevel)
		return;
	std::time_t now = std::time(NULL);
	char timebuf[32];
	std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

	std::ostream& out = (level >= ERROR) ? std::cerr : std::cout;

	if (_colorEnabled) {
		out << "\033[0;90m" << timebuf << "\033[0m "
			<< levelToColor(level) << "[" << levelToString(level) << "]\033[0m "
			<< message << std::endl;
	} else {
		out << timebuf << " [" << levelToString(level) << "] "
			<< message << std::endl;
	}
}

void Logger::debug(const std::string& message) { log(DEBUG, message); }
void Logger::info (const std::string& message) { log(INFO,  message); }
void Logger::warn (const std::string& message) { log(WARN,  message); }
void Logger::error(const std::string& message) { log(ERROR, message); }
