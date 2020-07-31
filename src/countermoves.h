#pragma once

#include "commons.h"

void AddCounterMove(int engineId, Move move, Move previousMove);

bool IsCounterMove(int engineId, Move move, Move previousMove);

void ClearCounterMoves();