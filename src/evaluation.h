#pragma once
#include "commons.h"

#define CASTLED 28
#define OPEN_ROOK_FILE 19
#define SEMI_OPEN_FILE 15
#define DOUBLE_PAWN 9
#define KING_NEEDS_PROTECTION 1500
#define KING_EXPOSED_INFRONT 16
#define KING_EXPOSED_DIAGONAL 12
#define PASSED_PAWN 7
#define PAWN_PROTECT 8
#define ENDGAME 1800
#define SAME_TWICE 18 //todo change to 18
#define QUEEN_EARLY 15 //todo change to 15

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

bool IsDraw(Game* game);

short KingExposed(int square, Game* game);

short PassedPawn(int square, Game* game);

short TotalMaterial(Game* game);
void CalculatePatterns();


