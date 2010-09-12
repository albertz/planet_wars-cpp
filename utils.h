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

struct Process {
	std::string cmd;
	bool running;
	int forkInputFd, forkOutputFd;
	std::string inbuffer, outbuffer;
	pid_t forkId;

	Process(const std::string& __cmd = "")
	: cmd(__cmd), running(false),
	forkInputFd(0), forkOutputFd(0), forkId(0) {}

	operator bool() const { return running; }
	void destroy();
	void run();
	
	Process& operator<<(const std::string& s) { inbuffer += s; return *this; }
	Process& operator<<(void (*func)(Process&)) { (*func)(*this); return *this; }

	bool readLine(std::string& s, size_t timeout = 0);

	void flush();
};

inline void flush(Process& p) { p.flush(); }
inline void endl(Process& p) { p << "\n"; p.flush(); }

#endif
