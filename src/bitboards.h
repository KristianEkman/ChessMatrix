#pragma once

#include "commons.h"

typedef struct
{
	U64 WhitePawns;
	U64 BlackPawns;
	U64 AllPawns;
} PawnBitboards;

typedef struct
{
	U64 WhiteKnights;
	U64 BlackKnights;
	U64 AllKnights;
} KnightBitboards;

typedef struct
{
	U64 WhiteBishops;
	U64 BlackBishops;
	U64 AllBishops;
} BishopBitboards;

typedef struct
{
	U64 WhiteRooks;
	U64 BlackRooks;
	U64 AllRooks;
} RookBitboards;

typedef struct
{
	U64 WhiteQueens;
	U64 BlackQueens;
	U64 AllQueens;
} QueenBitboards;

typedef struct
{
	U64 WhiteKing;
	U64 BlackKing;
	U64 AllKings;
} KingBitboards;

typedef struct
{
	PawnBitboards Pawns;
	KnightBitboards Knights;
	BishopBitboards Bishops;
	RookBitboards Rooks;
	QueenBitboards Queens;
	KingBitboards Kings;
	U64 WhitePieces;
	U64 BlackPieces;
	U64 Occupied;
    U64 Matrix[7][2]
} AllPieceBitboards;

U64 SquareToBit(int square);
PawnBitboards GetPawnBitboards(const Game *game);
KnightBitboards GetKnightBitboards(const Game *game);
BishopBitboards GetBishopBitboards(const Game *game);
RookBitboards GetRookBitboards(const Game *game);
QueenBitboards GetQueenBitboards(const Game *game);
KingBitboards GetKingBitboards(const Game *game);
AllPieceBitboards GetAllPieceBitboards(const Game *game);
