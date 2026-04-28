#ifndef STRINGUTILS_HPP
# define STRINGUTILS_HPP

# include <vector>
# include <string>

namespace StringUtils {

	std::string 				trim(const std::string& s);
	std::string 				ltrim(const std::string& s);
	std::string 				rtrim(const std::string& s);

	std::string 				toLower(const std::string& s);
	std::string 				toUpper(const std::string& s);

	std::vector<std::string> 	split(const std::string& s, char delimiter);
	std::vector<std::string> 	split(const std::string& s, const std::string& delimiter);

	bool						startsWith(const std::string& s, const std::string& prefix);
	bool						endsWith(const std::string& s, const std::string&suffix);

	std::string					toString(int n);
	std::string					toString(long n);
	std::string					toString(size_t n);

	bool						toInt(const std::string& s, int& out);
	bool						toSize(const std::string& s, size_t& out);
}

#endif
