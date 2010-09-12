// code under GPLv3
// by Albert Zeyer

#ifndef __PW__UTILS_H__
#define __PW__UTILS_H__

#include <string>
#include <sstream>
#include <list>

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

inline std::list<std::string>& operator+=(std::list<std::string>& l, const std::string& s) {
	l.push_back(s);
	return l;
}

struct Process {
	operator bool();
	void destroy();
	
};

#endif
