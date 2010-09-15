/*
 *  process.h
 *  PlanetWars
 *
 *  Created by Albert Zeyer on 15.09.10.
 *  code under GPLv3
 *
 */

#ifndef __PW__PROCESS_H__
#define __PW__PROCESS_H__

#include <string>
#include <unistd.h>

struct Process {
	std::string cmd;
	bool running;
	int forkInputFd, forkOutputFd;
	std::string inbuffer, outbuffer;
	pid_t forkId;
	
	Process(const std::string& __cmd = "")
	: cmd(__cmd), running(false),
	forkInputFd(0), forkOutputFd(0), forkId(0) {}
	~Process() { destroy(); }
	
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
