#pragma once
#include "commons.h"

bool SearchDeeper(int engineId, int currentDepth, int moveNo, int ellapsed, Side side);

void RegisterDepthTime(int engineId, int moveNo, int depth, int time);

void ResetDepthTimes();

void SetMoveTimeFallBack(Side side);
