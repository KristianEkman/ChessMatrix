#pragma once
#include "commons.h"

void AddWhiteKiller(Game* game, Move move);
void AddBlackKiller(Game* game, Move move);
bool KillerScore(Game* game, int side, Move move);
