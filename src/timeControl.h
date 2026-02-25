#pragma once
#include "commons.h"

bool SearchDeeper(int currentDepth, int moveNo, int elapsedMs, Side side);

void RegisterDepthTime(int moveNo, int depth, int time);

void RegisterIterationResult(int moveNo, int depth, Move bestMove, short score);

void ResetDepthTimes();

void SetMoveTimeFallBack(Side side);
