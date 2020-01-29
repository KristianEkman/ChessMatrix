#pragma once
#include "commons.h"

bool SearchDeeper(int currentDepth, int moveNo, int ellapsed, Side side);

void RegisterDepthTime(int moveNo, int depth, int time);

void ResetDepthTimes();

void SetMoveTimeFallBack(Side side);
