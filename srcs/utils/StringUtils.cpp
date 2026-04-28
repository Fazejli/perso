#include "utils/StringUtils.hpp"
#include <sstream>
#include <cctype>
#include <cstdlib>
#include <climits>
#include <cerrno>

namespace StringUtils {
	static const char* WHITESPACE= " \t\r\n\v\f";

	std::string ltrim(const std::string& s) {
		std::string::size_type start = s.find_first_not_of(WHITESPACE);
		if (start == std::string::npos) return "";
		return s.substr(start);
	}

	std::string rtrim(const std::string& s) {
		std::string::size_type end = s.find_last_not_of(WHITESPACE);
		if (end == std::string::npos) return "";
		return s.substr(0, end + 1);
	}

	std::string trim(const std::string& s) {
		return rtrim(ltrim(s));
	}

	std::string toLower(const std::string& s) {
		std::string out(s);
		for (std::string::size_type i = 0; i < out.size(); i++)
			out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
		return out;
	}

	std::string toUpper(const std::string& s) {
		std::string out(s);
		for (std::string::size_type i = 0; i < out.size(); i++)
			out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[i])));
		return out;
	}

	std::vector<std::string> split(const std::string& s, char delimiter) {
		std::vector<std::string> tokens;
		std::string::size_type start = 0;
		std::string::size_type pos;
		while ((pos = s.find(delimiter, start)) != std::string::npos) {
			tokens.push_back(s.substr(start, pos - start));
			start = pos + 1;
		}
		tokens.push_back(s.substr(start));
		return tokens;
	}

	std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
		std::vector<std::string> tokens;
		if (delimiter.empty()) {
			tokens.push_back(s);
			return tokens;
		}
		std::string::size_type start = 0;
		std::string::size_type pos;
		while ((pos = s.find(delimiter, start)) != std::string::npos) {
			tokens.push_back(s.substr(start, pos - start));
			start = pos + delimiter.size();
		}
		tokens.push_back(s.substr(start));
		return tokens;
	}

	bool startsWith(const std::string& s, const std::string& prefix){
		if (prefix.size() > s.size()) return false;
		return s.compare(0, prefix.size(), prefix) == 0;
	}

	bool endsWith(const std::string& s, const std::string& suffix) {
		if (suffix.size() > s.size()) return false;
		return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
	}

	std::string toString(int n) {
		std::ostringstream oss;
		oss << n;
		return oss.str();
	}

	std::string toString(long n) {
		std::ostringstream oss;
		oss << n;
		return oss.str();
	}

	std::string toString(size_t n) {
		std::ostringstream oss;
		oss << n;
		return oss.str();
	}

	bool toInt(const std::string& s, int& out) {
    	if (s.empty()) return false;
    	char* end = NULL;
    	errno = 0;
    	long val = std::strtol(s.c_str(), &end, 10);
    	if (end == s.c_str() || *end != '\0') return false;
    	if (errno == ERANGE) return false;
    	if (val < INT_MIN || val > INT_MAX) return false;
    	out = static_cast<int>(val);
    	return true;
	}

	bool toSize(const std::string& s, size_t& out) {
    	if (s.empty()) return false;
    	char* end = NULL;
    	errno = 0;
    	long val = std::strtol(s.c_str(), &end, 10);
    	if (end == s.c_str() || *end != '\0') return false;
    	if (errno == ERANGE || val < 0) return false;
    	out = static_cast<size_t>(val);
    	return true;
	}
}
