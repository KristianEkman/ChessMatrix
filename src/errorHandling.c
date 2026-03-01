#include "errorHandling.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#include <execinfo.h>
#endif

static volatile sig_atomic_t g_QuitRequested = 0;

static void IgnoreWriteResult(int writeResult)
{
    (void)writeResult;
}

void EmitUciError(const char *message)
{
    printf("info string error %s\n", message);
    fflush(stdout);
}

static void WriteUnhandledErrorLine(const char *line, unsigned int len)
{
#ifdef _WIN32
    IgnoreWriteResult(_write(1, line, len));
    IgnoreWriteResult(_write(2, line, len));
#else
    IgnoreWriteResult((int)write(STDOUT_FILENO, line, len));
    IgnoreWriteResult((int)write(STDERR_FILENO, line, len));
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
        EmitUciError("process exited without quit command");
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

void MarkQuitRequested()
{
    g_QuitRequested = 1;
}
