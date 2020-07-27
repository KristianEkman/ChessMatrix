#pragma once

#include "commons.h"

void AddCounterMove(Move move, Move previousMove);

bool IsCounterMove(Move move, Move previousMove);

void ClearCounterMoves();