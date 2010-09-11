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

#endif
