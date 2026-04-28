#ifndef LOGGER_HPP
# define LOGGER_HPP

# include <string>

class Logger {
	public :
		enum Level {
			DEBUG = 0,
			INFO = 1,
			WARN = 2,
			ERROR = 3
		};

		static void log(Level level, const std::string& message);
		static void debug(const std::string& message);
		static void info(const std::string& message);
		static void warn(const std::string& message);
		static void error(const std::string& message);

		static void setLevel(Level level);
		static void setColorEnabled(bool enabled);

	private :
		Logger();
		Logger(const Logger& other);
		Logger& operator=(const Logger& other);
		~Logger();

		static const char* levelToString(Level level);
		static const char* levelToColor(Level level);

		static Level _minLevel;
		static bool _colorEnabled;
};

#endif
