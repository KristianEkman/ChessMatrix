#include "platform.h"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <conio.h>
#include <io.h>

bool PlatformCreateThread(PlatformThread *thread, PlatformThreadFunc func, void *arg)
{
	*thread = CreateThread(NULL, 0, func, arg, 0, NULL);
	return *thread != NULL;
}

void PlatformJoinThread(PlatformThread thread)
{
	WaitForSingleObject(thread, INFINITE);
	CloseHandle(thread);
}

void PlatformDetachThread(PlatformThread thread)
{
	CloseHandle(thread);
}

void PlatformSleepMs(int ms)
{
	Sleep(ms);
}

int PlatformGetChar()
{
	return _getch();
}

void PlatformClearScreen()
{
	system("@cls||clear");
}

#else

#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <execinfo.h>

bool PlatformCreateThread(PlatformThread *thread, PlatformThreadFunc func, void *arg)
{
	return pthread_create(thread, NULL, (void *(*)(void *))func, arg) == 0;
}

void PlatformJoinThread(PlatformThread thread)
{
	pthread_join(thread, NULL);
}

void PlatformDetachThread(PlatformThread thread)
{
	pthread_detach(thread);
}

void PlatformSleepMs(int ms)
{
	if (ms > 0)
	{
		struct timespec req;
		struct timespec rem;
		req.tv_sec = ms / 1000;
		req.tv_nsec = (long)(ms % 1000) * 1000000L;
		while (nanosleep(&req, &rem) == -1 && errno == EINTR)
			req = rem;
	}
}

int PlatformGetChar()
{
	struct termios oldt;
	struct termios newt;
	int ch;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= (unsigned int)~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}

void PlatformClearScreen()
{
	int result = system("clear");
	(void)result;
}

#endif


