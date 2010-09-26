/*
 *  process_win32.cpp
 *  PlanetWars
 *
 *  Created by Albert Zeyer on 15.09.10.
 *  ported to Win32 by theli_ua
 *  code under GPLv3
 *
 */

#ifdef _WIN32

#include <iostream>
#include <vector>
#include <string>
#include <stdio.h> // strerror etc
#include <errno.h>
#include <signal.h> // kill etc
#include <fcntl.h>
#include <string.h>
#include "process.h"
#include "utils.h"

#define BUFSIZE 4096 

void ErrorExit(PTSTR lpszFunction) 

// Format a readable error message, display a message box, 
// and exit from the application.
{ 
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
	StringCchPrintf((LPTSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"), 
		lpszFunction, dw, lpMsgBuf); 
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(1);
}

void Process::run() {
	SECURITY_ATTRIBUTES saAttr; 

	// Set the bInheritHandle flag so pipe handles are inherited. 
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 

	// Create a pipe for the child process's STDOUT. 
	if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) ) 
		ErrorExit(TEXT("StdoutRd CreatePipe")); 

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
		ErrorExit(TEXT("Stdout SetHandleInformation")); 

	// Create a pipe for the child process's STDIN. 
	if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) 
		ErrorExit(TEXT("Stdin CreatePipe")); 

	// Ensure the write handle to the pipe for STDIN is not inherited. 
	if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
		ErrorExit(TEXT("Stdin SetHandleInformation")); 


	std::string mycmd = "cmd.exe /D /c " + cmd;

	//Create child process


	PROCESS_INFORMATION piProcInfo; 
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE; 

	// Set up members of the PROCESS_INFORMATION structure. 

	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.

	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
	siStartInfo.cb = sizeof(STARTUPINFO); 
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Create the child process. 

	bSuccess = CreateProcess(
		NULL, 
		(LPSTR)cmd.c_str(),     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 

	// If an error occurs, exit the application. 
	if ( ! bSuccess ) 
		ErrorExit(TEXT("CreateProcess"));
	else 
	{
		running = true;

		//CloseHandle(piProcInfo.hProcess);
		hProcess = piProcInfo.hProcess;
		CloseHandle(piProcInfo.hThread);
	}


}

void Process::destroy() {
	CloseHandle( g_hChildStd_IN_Rd);
	CloseHandle( g_hChildStd_IN_Wr);
	CloseHandle( g_hChildStd_OUT_Rd);
	CloseHandle( g_hChildStd_OUT_Wr);

	if (hProcess)
		TerminateProcess(hProcess,0);
	
	CloseHandle(hProcess);
	running = false;
}

void Process::waitForExit() {
	if (running)
		WaitForSingleObject(hProcess, INFINITE);
}

static timeval millisecsToTimeval(size_t ms) {
	timeval v;
	v.tv_sec = ms / 1000;
	v.tv_usec = (ms % 1000) * 1000;
	return v;
}

bool Process::readLine(std::string& s, size_t timeout) {
	DWORD dwRead = 0; 
	CHAR chBuf; 
	BOOL bSuccess = FALSE;

	for (;;) 
	{ 
		bSuccess = ReadFile( g_hChildStd_OUT_Rd, &chBuf, 1, &dwRead, NULL);
		if( ! bSuccess || dwRead == 0 )
		{
			outbuffer = "";
			return false;
		}
		else
		{
			outbuffer += chBuf;
		}

		if (outbuffer[outbuffer.size() - 1] == '\n')
		{
			if (outbuffer[outbuffer.size() - 2] == '\r')
				outbuffer.resize(outbuffer.size() - 2);
			else
				outbuffer.resize(outbuffer.size() - 1);
			s = outbuffer;
			outbuffer = "";
			return true;
		}

	} 
}
void Process::flush() {
	DWORD dwRead = inbuffer.size(), dwWritten; 
	BOOL bSuccess = FALSE;




	size_t n = 0;
	while(n < inbuffer.size() && (bSuccess = WriteFile(g_hChildStd_IN_Wr, inbuffer.c_str() + n, dwRead, &dwWritten, NULL)))
	{
		n += dwWritten;
		dwRead -= dwWritten;
	}


	inbuffer.resize(0);
}

#endif // _WIN32
