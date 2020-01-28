#pragma once
#include <stdlib.h>
#include "commons.h"
#include <Windows.h>

#define SEARCH_THREADS 7

uint g_SearchedNodes;
bool g_Stopped;
TopSearchParams g_topSearchParams;
Game g_threadGames[SEARCH_THREADS];
GlobalRootMoves g_rootMoves;
HANDLE g_MutexFreeMove;

short AlphaBeta(short alpha, short beta, int depth, int captIndex, Game* game, bool doNull, short moveScore, int deep_in);
Move Search(bool async);
