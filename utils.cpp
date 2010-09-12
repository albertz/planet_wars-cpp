// code under GPLv3
// by Albert Zeyer

#include "utils.h"
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

long currentTimeMillis() {
	struct Timeval : timeval {
		Timeval() {
			gettimeofday(this, NULL);
		}
	};
	static Timeval start;

	struct timeval end;
	gettimeofday(&end, NULL);

	long mtime, seconds, useconds;
	seconds  = end.tv_sec  - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;

	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
	return mtime;
}

typedef unsigned char uchar;

std::string TrimSpaces(const std::string& str) {	
	size_t start = 0;
	for(size_t i = 0; i < str.size(); ++i)
		if(!isspace((uchar)str[i]) || isgraph((uchar)str[i])) {
			start = i;
			break;
		}
	
	size_t n = 0;
	for(size_t i = str.size(); i > start; --i)
		if(!isspace((uchar)str[i-1]) || isgraph((uchar)str[i-1])) {
			n = i - start - 1;
			break;
		}
	
	return str.substr(start, n);
}

std::string ToLower(const std::string& str) {
	std::string res;
	for(std::string::const_iterator i = str.begin(); i != str.end(); i++)
		res += tolower((uchar)*i);
	return res;
}
