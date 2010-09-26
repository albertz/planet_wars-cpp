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

#ifdef _WIN32
#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#else
#include <unistd.h>
#endif

struct Process {
	std::string cmd;
	bool running;
	int forkInputFd, forkOutputFd;
	std::string inbuffer, outbuffer;

#ifdef _WIN32
	HANDLE g_hChildStd_IN_Rd;
	HANDLE g_hChildStd_IN_Wr;
	HANDLE g_hChildStd_OUT_Rd;
	HANDLE g_hChildStd_OUT_Wr;
	HANDLE hProcess;
#else
	pid_t forkId;
#endif
	
	Process(const std::string& __cmd = "")
	: cmd(__cmd), running(false),
#ifdef _WIN32
	g_hChildStd_IN_Rd(NULL),
	g_hChildStd_IN_Wr(NULL),
	g_hChildStd_OUT_Rd(NULL),
	g_hChildStd_OUT_Wr(NULL),
	hProcess(NULL)
#else
	forkInputFd(0), forkOutputFd(0), forkId(0) 
#endif
	{}
	~Process() { destroy(); }
	
	operator bool() const { return running; }
	void destroy();
	void waitForExit();
	void run();
	
	Process& operator<<(const std::string& s) { inbuffer += s; return *this; }
	Process& operator<<(void (*func)(Process&)) { (*func)(*this); return *this; }
	
	bool readLine(std::string& s, size_t timeout = 0);
	
	void flush();
};

inline void flush(Process& p) { p.flush(); }
inline void endl(Process& p) { p << "\n"; p.flush(); }

#endif
