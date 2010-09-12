// code under GPLv3
// by Albert Zeyer

#include "utils.h"
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

long getMilliseconds() {
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
