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

volatile sig_atomic_t g_QuitRequested = 0;

static void WriteUnhandledErrorLine(const char *line, unsigned int len)
{
#ifdef _WIN32
	_write(1, line, len);
	_write(2, line, len);
#else
	write(STDOUT_FILENO, line, len);
	write(STDERR_FILENO, line, len);
#endif
}

#ifndef _WIN32
static void WriteSignalStackTrace()
{
	void *frames[64];
	int frameCount = backtrace(frames, 64);
	if (frameCount > 0)
	{
		WriteUnhandledErrorLine("info string error stacktrace begin\n", (unsigned int)(sizeof("info string error stacktrace begin\n") - 1));
		backtrace_symbols_fd(frames, frameCount, STDOUT_FILENO);
		backtrace_symbols_fd(frames, frameCount, STDERR_FILENO);
		WriteUnhandledErrorLine("info string error stacktrace end\n", (unsigned int)(sizeof("info string error stacktrace end\n") - 1));
	}
}
#endif

static void HandleUnhandledErrorSignal(int signalNumber)
{
	switch (signalNumber)
	{
	case SIGABRT:
		WriteUnhandledErrorLine("info string error unhandled signal SIGABRT\n", (unsigned int)(sizeof("info string error unhandled signal SIGABRT\n") - 1));
		break;
	case SIGFPE:
		WriteUnhandledErrorLine("info string error unhandled signal SIGFPE\n", (unsigned int)(sizeof("info string error unhandled signal SIGFPE\n") - 1));
		break;
	case SIGILL:
		WriteUnhandledErrorLine("info string error unhandled signal SIGILL\n", (unsigned int)(sizeof("info string error unhandled signal SIGILL\n") - 1));
		break;
	case SIGSEGV:
		WriteUnhandledErrorLine("info string error unhandled signal SIGSEGV\n", (unsigned int)(sizeof("info string error unhandled signal SIGSEGV\n") - 1));
		break;
#ifdef SIGBUS
	case SIGBUS:
		WriteUnhandledErrorLine("info string error unhandled signal SIGBUS\n", (unsigned int)(sizeof("info string error unhandled signal SIGBUS\n") - 1));
		break;
#endif
	default:
		WriteUnhandledErrorLine("info string error unhandled signal UNKNOWN\n", (unsigned int)(sizeof("info string error unhandled signal UNKNOWN\n") - 1));
		break;
	}
#ifndef _WIN32
	WriteSignalStackTrace();
#endif
	_Exit(EXIT_FAILURE);
}

static void OnProcessExit()
{
	if (!g_QuitRequested)
	{
		printf("info string error process exited without quit command\n");
		fflush(stdout);
	}
}

void InstallUnhandledErrorHandlers()
{
	atexit(OnProcessExit);
	signal(SIGABRT, HandleUnhandledErrorSignal);
	signal(SIGFPE, HandleUnhandledErrorSignal);
	signal(SIGILL, HandleUnhandledErrorSignal);
	signal(SIGSEGV, HandleUnhandledErrorSignal);
#ifdef SIGTERM
	signal(SIGTERM, HandleUnhandledErrorSignal);
#endif
#ifdef SIGINT
	signal(SIGINT, HandleUnhandledErrorSignal);
#endif
#ifdef SIGPIPE
	signal(SIGPIPE, HandleUnhandledErrorSignal);
#endif
#ifdef SIGBUS
	signal(SIGBUS, HandleUnhandledErrorSignal);
#endif
}
