#pragma once

#include "commons.h"

#ifdef _MSC_VER
#include <intrin.h>
#endif

static inline int popcount(U64 bitboard)
{
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_popcountll(bitboard);
#elif defined(_MSC_VER)
	return (int)__popcnt64(bitboard);
#else
	int count = 0;
	while (bitboard)
	{
		bitboard &= (bitboard - 1);
		count++;
	}
	return count;
#endif
}

static inline int pop_lsb(U64 *bitboard)
{
	if (bitboard == 0 || *bitboard == 0ULL)
		return -1;

#if defined(__clang__) || defined(__GNUC__)
	int square = __builtin_ctzll(*bitboard);
#elif defined(_MSC_VER)
	unsigned long square;
	_BitScanForward64(&square, *bitboard);
#else
	int square = 0;
	U64 bits = *bitboard;
	while ((bits & 1ULL) == 0ULL)
	{
		bits >>= 1;
		square++;
	}
#endif

	*bitboard &= (*bitboard - 1);
	return square;
}

static inline U64 SquareBitUnchecked(int square)
{
	return 1ULL << square;
}

U64 SquareToBit(int square);
void AddPieceToBitboards(AllPieceBitboards *bitboards, PieceType pieceType, int square);
void RemovePieceFromBitboards(AllPieceBitboards *bitboards, PieceType pieceType, int square);
void MovePieceOnBitboards(AllPieceBitboards *bitboards, PieceType pieceType, int from, int to);
void ReplacePieceOnBitboards(AllPieceBitboards *bitboards, PieceType fromPiece, PieceType toPiece, int square);
void SyncGameBitboards(Game *game);
PawnBitboards GetPawnBitboards(const Game *game);
KnightBitboards GetKnightBitboards(const Game *game);
BishopBitboards GetBishopBitboards(const Game *game);
RookBitboards GetRookBitboards(const Game *game);
QueenBitboards GetQueenBitboards(const Game *game);
KingBitboards GetKingBitboards(const Game *game);
AllPieceBitboards GetAllPieceBitboards(const Game *game);
