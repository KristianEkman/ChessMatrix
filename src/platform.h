#pragma once

#include "commons.h"

#ifdef _WIN32
#include <Windows.h>

typedef HANDLE PlatformThread;
typedef DWORD PlatformThreadReturn;
#define PLATFORM_THREAD_CALL WINAPI
#define PLATFORM_THREAD_RETURN_VALUE 0
#else
#include <pthread.h>

typedef pthread_t PlatformThread;
typedef void* PlatformThreadReturn;
#define PLATFORM_THREAD_CALL
#define PLATFORM_THREAD_RETURN_VALUE NULL
#endif

typedef PlatformThreadReturn(PLATFORM_THREAD_CALL* PlatformThreadFunc)(void*);

bool PlatformCreateThread(PlatformThread* thread, PlatformThreadFunc func, void* arg);
void PlatformJoinThread(PlatformThread thread);
void PlatformDetachThread(PlatformThread thread);

void PlatformSleepMs(int ms);
int PlatformGetChar();
void PlatformClearScreen();

#include <signal.h>

extern volatile sig_atomic_t g_QuitRequested;
void InstallUnhandledErrorHandlers(void);
