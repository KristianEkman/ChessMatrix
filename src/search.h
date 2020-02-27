#pragma once
#include <stdlib.h>
#include <Windows.h>
#include "commons.h"

#define SEARCH_THREADS 1

uint g_SearchedNodes;
bool g_Stopped;
TopSearchParams g_topSearchParams;
Game g_threadGames[SEARCH_THREADS];
GlobalRootMoves g_rootMoves;
HANDLE g_MutexFreeMove;

void DefaultSearch();
Move Search(bool async);
