#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

# include <string>
# include <stdexcept>
# include "config/Config.hpp"

// Reads a configuration file from disk and produces a Config object.
// Throws ParseError on syntactic or semantic problems.

// NOTE: this class currently only verifies that the file exists and
// returns an empty Config. The real parser will be implemented after.

class ConfigParser {
	public :
		class ParseError : public std::runtime_error {
			public :
				explicit ParseError(const std::string& msg);
		};

		ConfigParser();
		~ConfigParser();

		Config parse(const std::string& filepath);

	private :
		ConfigParser(const ConfigParser& other);
		ConfigParser& operator=(const ConfigParser& other);

	// We will introduce: tokenizer, recursive descent parser,
    // server-block / location-block parsers, directive validators, etc... later
};

#endif
