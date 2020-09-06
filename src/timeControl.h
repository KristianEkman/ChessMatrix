#pragma once
#include "commons.h"

bool SearchDeeper(int currentDepth, int moveNo, int ellapsed, int side);

void RegisterDepthTime(int moveNo, int depth, int time);

void ResetDepthTimes();

void SetMoveTimeFallBack(int side);
