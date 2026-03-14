#pragma once

#include "commons.h"

U64 SquareToBit(int square);
void SyncGameBitboards(Game *game);
PawnBitboards GetPawnBitboards(const Game *game);
KnightBitboards GetKnightBitboards(const Game *game);
BishopBitboards GetBishopBitboards(const Game *game);
RookBitboards GetRookBitboards(const Game *game);
QueenBitboards GetQueenBitboards(const Game *game);
KingBitboards GetKingBitboards(const Game *game);
AllPieceBitboards GetAllPieceBitboards(const Game *game);
