#pragma once
#include "commons.h"

#define CASTLED 38
#define OPEN_ROOK_FILE 19
#define SEMI_OPEN_FILE 12
#define DOUBLE_PAWN 12
#define KING_EXPOSED 20
#define PASSED_PAWN_FREE_PATH 15
#define PAWN_PROTECT 8
#define SAME_TWICE 15
#define QUEEN_EARLY 15
#define BISHOP_MOBILITY 3
#define BISHOP_PAIR 30

//[type][side][square]
extern short PositionValueMatrix[7][2][64];

//[side][short/long]
extern short CastlingPoints[2];

//[middle or end][side][square]
extern short KingPositionValueMatrix[2][2][64];

extern U64 FileMask[8];
extern U64 AdjacentFileMask[8];
extern U64 PassedPawnMask[2][64];
extern U64 PawnProtectorsMask[2][64];
extern U64 KingShieldMask[2][64];

short OpenRookFile(int square, Game* game, PieceType rook);

short DoublePawns(int square, Game* game, PieceType pawn);

short GetMoveOrderingScore(Move move, Game *game);

short GetEval(Game* game);

bool IsDraw(Game* game);

short KingExposed(int square, Game* game);

short PassedPawn(int square, Game* game);
short PassedPawnRaceBonus(int square, Game *game);

short ProtectedByPawn(int square, Game* game);

short BishopMobility(int square, Game* game);

short TotalMaterial(Game* game);
int GetGamePhase(Game* game);
short GetKingPositionScore(Move move, Game *game);
void CalculatePatterns();
void AdjustPositionImportance();
void SwitchSignOfWhitePositionValue();


