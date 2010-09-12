// code under GPLv3
// by Albert Zeyer

#ifndef __PW__UTILS_H__
#define __PW__UTILS_H__

#include <string>
#include <sstream>

template<typename T>
std::string to_string(T val) {
	std::ostringstream oss;
	oss << val;
	return oss.str();
}

long currentTimeMillis();

template<typename T>
bool AllTrue(const T& container) {
	for(typename T::const_iterator i = container.begin(); i != container.end(); ++i)
		if(!*i) return false;
	return true;
}

std::string TrimSpaces(const std::string& str);
std::string	ToLower(const std::string& str);

struct Process {
	operator bool();
	void destroy();
	
};

#endif
