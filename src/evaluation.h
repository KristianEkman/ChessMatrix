#pragma once
#include "commons.h"

//[type][side][square]
short PositionValueMatrix[7][2][64];

//[side][short/long]
short CastlingPoints[2];

//[middle or end][side][square]
short KingPositionValueMatrix[2][2][64];

short OpenRookFile(int square, Game* game);

short DoublePawns(int square, Game* game, PieceType pawn);

// A much faster calculation just used for move ordering.
short GetMoveScore(Game* game);

short GetEval(Game* game, short moveScore);

bool DrawByRepetition(Game* game);

short KingExposed(int square, Game* game);

short PassedPawn(int square, Game* game);

short TotalMaterial(Game* game);


