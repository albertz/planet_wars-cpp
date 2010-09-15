// code under GPLv3
// by Albert Zeyer

#ifndef __PW__UTILS_H__
#define __PW__UTILS_H__

#include <string>
#include <sstream>
#include <list>
#include <vector>
#include <ostream>

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

void Tokenize(const std::string& s,
			  const std::string& delimiters,
			  std::vector<std::string>& tokens);
	
inline
std::vector<std::string>
Tokenize(const std::string& s, const std::string& delimiters = " ") {
	std::vector<std::string> tokens;
	Tokenize(s, delimiters, tokens);
	return tokens;	
}


inline std::list<std::string>& operator+=(std::list<std::string>& l, const std::string& s) {
	l.push_back(s);
	return l;
}

template<typename A, typename B, typename C>
std::basic_ostream<A,B>& operator<<(std::basic_ostream<A,B>& _s, const std::list<C>& l) {
	std::basic_ostream<A,B>* s = &_s;
	for(typename std::list<C>::const_iterator i = l.begin(); i != l.end(); ++i)
		s = &( *s << *i );
	return *s;
}

template <typename T> T CLAMP(const T& num, const T& lower_bound, const T& upper_bound) {
	return (num < lower_bound) ? lower_bound : (num > upper_bound ? upper_bound : num); }

template <typename T> T SIGN(const T& num) { return (num < 0) ? -1 : 1; }

#endif
