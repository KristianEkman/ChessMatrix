#include "commons.h"
#include "evaluation.h"
#include "bitboards.h"
#include "patterns.h"
#include <stdlib.h>


//#define MOBILITY 3 // for every square a rook or bishop can go to

#if defined(_MSC_VER)
#define CM_THREAD_LOCAL __declspec(thread)
#else
#define CM_THREAD_LOCAL _Thread_local
#endif

#define PAWN_HASH_SIZE 16384

typedef struct
{
	U64 Key;
	short Score[2];
	uchar PawnCount[2];
} PawnHashEntry;

static const int PawnPassedPoints[2][8];

//white, black, (flipped, starts at A1)
//[type][side][square]
short PositionValueMatrix[7][2][64] = {
	{
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, },
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, }
	},
	// bishopPositionValues
	{
		{
			0,  0,  0,  0,  0,  0,  0, 0,
			0,  5,  0,  0,  0,  0,  5, 0,
			0,  0, 10, 10, 10, 10,  0, 0,
			0,  0, 10, 10, 10, 10,  0, 0,
			0,  0,  5, 10, 10,  5,  0, 0,
			0,  0,  5, 10, 10,  5,  0, 0,
			0,  0,  0,  0,  0,  0,  0, 0,
			0,  0,  0,  0,  0,  0,  0, 0,
		},{
			0,  0,  0,  0,  0,  0, 0, 0,
			0,  0,  0,  0,  0,  0,  0, 0,
			0,  0,  5, 10, 10,  5,  0, 0,
			0,  5,  5, 10, 10,  5,  5, 0,
			0,  0, 10, 10, 10, 10,  0, 0,
			0,  0, 10, 10, 10, 10,  0, 0,
			0,  5,  0,  0,  0,  0,  5, 0,
			0,  0,  0,  0,  0,  0,  0, 0
		}
	},
	//rookPositionValues[2][64] =
	{
		{
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			20,20,20,20,20,20,20,20,
			0, 0, 5,10,10, 5, 0, 0,
		},
		{
			0, 0, 5,10,10, 5, 0, 0,
		   20,20,20,20,20,20,20,20,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
		}
	},
	// queenPositionValues[2][64] = 
	{
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
		}
	},
	//char pawnPositionValues[2][64] =
	{
		{
			0,  0,  0,  0,  0,  0,  0,  0,
		   10, 10,  0,-10,-10,  0, 10, 10,
			5,  0,  0,  5,  5,  0,  0,  5,
			0,  0, 10, 20, 20, 10,  0,  0,
			5,  5,  5, 10, 10,  5,  5,  5,
		   10, 10, 10, 20, 20, 10, 10, 10,
		   20, 20, 20, 30, 30, 20, 20, 20,
			0,  0,  0,  0,  0,  0,  0,  0
   },
		{
			0,  0,  0,  0,  0,  0,  0,  0,
		   20, 20, 20, 30, 30, 20, 20, 20,
		   10, 10, 10, 20, 20, 10, 10, 10,
			5,  5,  5, 10, 10,  5,  5,  5,
			0,  0, 10, 20, 20, 10,  0,  0,
			5,  0,  0,  5,  5,  0,  0,  5,
		   10, 10,  0,-10,-10,  0, 10, 10,
			0,  0,  0,  0,  0,  0,  0,  0,
		}
	},
	//char knightPositionsValues[2][64] =
	{
		{
			-20,-10,-10,-10,-10,-10,-10,-20,
			-10,-10,  0,  5,  5,  0,-10,-10,
			-10,  5, 10, 15, 15, 10,  5,-10,
			-10,  0, 15, 20, 20, 15,  0,-10,
			-10,  5, 15, 20, 20, 15,  5,-10,
			-10,  0, 10, 15, 15, 10,  0,-10,
			-10,-10,  0,  0,  0,  0,-10,-10,
			-20,-10,-10,-10,-10,-10,-10,-20
		},{
			-20,-10,-10,-10,-10,-10,-10,-20,
			-10,-10,  0,  5,  5,  0,-10,-10,
			-10,  5, 10, 15, 15, 10,  5,-10,
			-10,  0, 15, 20, 20, 15,  0,-10,
			-10,  5, 15, 20, 20, 15,  5,-10,
			-10,  0, 10, 15, 15, 10,  0,-10,
			-10,-10,  0,  0,  0,  0,-10,-10,
			-20,-10,-10,-10,-10,-10,-10,-20
		}
	},
	//filler for kings. they depend on age of game. i.e. end game
	{
		{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, },
		{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, }
	}
};

//[middle or end][side][square]
short KingPositionValueMatrix[2][2][64] = {
	//kingMiddleGamePositionValues[2][64] =
	{
		{
			  0,  5,  5,-10,-10,  0, 10, 5,
			-20,-20,-20,-20,-20,-20,-20,-20,
			-20,-20,-20,-20,-20,-20,-20,-20,
			-20,-30,-30,-40,-40,-30,-30,-20,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30
		},
		{
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-20,-30,-30,-40,-40,-30,-30,-20,
			-20,-20,-20,-20,-20,-20,-20,-20,
			-20,-20,-20,-20,-20,-20,-20,-20,
			  0,  5,  5,-10,-10,  0, 10,  5
		}
	},
	//kingEndGamePositionValues[2][64] = 
	{
		{
			-30,-20,-10,  0,  0,-10,-20,-30,
			-20,-10,  0,  5,  5,  0,-10,-20,
			-10,  0, 20, 30, 30, 20,  0,-10,
			  0,  5, 30, 40, 40, 30,  5,  0,
			  0,  5, 30, 40, 40, 30,  5,  0,
			-10,  0, 20, 30, 30, 20,  0,-10,
			-20,-10,  0,  5,  5,  0,-10,-20,
			-30,-20,-10,  0,  0,-10,-20,-30
		},{
			-30,-20,-10,  0,  0,-10,-20,-30,
			-20,-10,  0,  5,  5,  0,-10,-20,
			-10,  0, 20, 30, 30, 20,  0,-10,
			  0,  5, 30, 40, 40, 30,  5,  0,
			  0,  5, 30, 40, 40, 30,  5,  0,
			-10,  0, 20, 30, 30, 20,  0,-10,
			-20,-10,  0,  5,  5,  0,-10,-20,
			-30,-20,-10,  0,  0,-10,-20,-30
		}
	}
};

short CastlingPoints[2] = { -CASTLED, CASTLED };

U64 FileMask[8] = { 0 };
U64 AdjacentFileMask[8] = { 0 };
U64 PassedPawnMask[2][64] = { 0 };
U64 PawnProtectorsMask[2][64] = { 0 };
U64 KingShieldMask[2][64] = { 0 };

static CM_THREAD_LOCAL PawnHashEntry g_pawnHashTable[PAWN_HASH_SIZE] = { 0 };

// Array of coordinates for squares that could have a protecting pawn
char PawnProtectionSquares[2][64][3] = { 0 };

static U64 EvalSquareBit(int square)
{
	if (square < 0 || square > 63)
		return 0ULL;
	return 1ULL << square;
}

static U64 MixPawnHashComponent(U64 value)
{
	value ^= value >> 30;
	value *= 0xbf58476d1ce4e5b9ULL;
	value ^= value >> 27;
	value *= 0x94d049bb133111ebULL;
	value ^= value >> 31;
	return value;
}

static U64 GetPawnHashKey(const AllPieceBitboards *bb)
{
	U64 key = MixPawnHashComponent(bb->Pawns.WhitePawns) ^ (MixPawnHashComponent(bb->Pawns.BlackPawns) << 1);
	return key != 0ULL ? key : 1ULL;
}

static U64 GetSidePawns(const AllPieceBitboards *bb, int side01)
{
	return side01 == 0 ? bb->Pawns.WhitePawns : bb->Pawns.BlackPawns;
}

static U64 GetOpponentPieces(const AllPieceBitboards *bb, int side01)
{
	return side01 == 0 ? bb->BlackPieces : bb->WhitePieces;
}

static short GetPassedPawnBaseScore(int square, int side01, U64 opponentPawns)
{
	if (opponentPawns & PassedPawnMask[side01][square])
		return 0;
	return PawnPassedPoints[side01][square >> 3];
}

static short GetPassedPawnFreePathBonus(int square, int side01, const AllPieceBitboards *bb, U64 opponentPawns)
{
	int file = square & 7;
	int rank = square >> 3;
	U64 fileAheadMask = 0ULL;
	U64 opponentNonPawns = GetOpponentPieces(bb, side01) & ~opponentPawns;

	if (side01 == 0)
	{
		int shift = (rank + 1) * 8;
		if (shift < 64)
			fileAheadMask = FileMask[file] & (~0ULL << shift);
	}
	else
	{
		int shift = rank * 8;
		if (shift > 0)
			fileAheadMask = FileMask[file] & ((1ULL << shift) - 1ULL);
	}

	return (opponentNonPawns & fileAheadMask) == 0ULL ? PASSED_PAWN_FREE_PATH : 0;
}

static void GetPawnEval(const AllPieceBitboards *bb, short scores[2], uchar pawnCount[2])
{
	U64 key = GetPawnHashKey(bb);
	PawnHashEntry *entry = &g_pawnHashTable[key & (PAWN_HASH_SIZE - 1)];

	if (entry->Key == key)
	{
		scores[0] = entry->Score[0];
		scores[1] = entry->Score[1];
		pawnCount[0] = entry->PawnCount[0];
		pawnCount[1] = entry->PawnCount[1];
	}
	else
	{
		for (int side01 = 0; side01 < 2; side01++)
		{
			U64 ownPawns = GetSidePawns(bb, side01);
			U64 opponentPawns = GetSidePawns(bb, !side01);
			U64 pawns = ownPawns;
			short score = 0;

			pawnCount[side01] = (uchar)popcount(ownPawns);
			while (pawns)
			{
				int square = pop_lsb(&pawns);
				if (ownPawns & SquareToBit(square + Behind[side01]))
					score -= DOUBLE_PAWN;
				score += (short)(popcount(ownPawns & PawnProtectorsMask[side01][square]) * PAWN_PROTECT);
				score += GetPassedPawnBaseScore(square, side01, opponentPawns);
			}

			scores[side01] = score;
		}

		entry->Key = key;
		entry->Score[0] = scores[0];
		entry->Score[1] = scores[1];
		entry->PawnCount[0] = pawnCount[0];
		entry->PawnCount[1] = pawnCount[1];
	}

	for (int side01 = 0; side01 < 2; side01++)
	{
		U64 ownPawns = GetSidePawns(bb, side01);
		U64 opponentPawns = GetSidePawns(bb, !side01);
		U64 pawns = ownPawns;

		while (pawns)
		{
			int square = pop_lsb(&pawns);
			if (GetPassedPawnBaseScore(square, side01, opponentPawns) == 0)
				continue;
			scores[side01] += GetPassedPawnFreePathBonus(square, side01, bb, opponentPawns);
		}
	}
}

static void CalculateFileMasks(void)
{
	for (int file = 0; file < 8; file++)
	{
		U64 fileMask = 0ULL;
		U64 adjacentMask = 0ULL;

		for (int rank = 0; rank < 8; rank++)
		{
			fileMask |= EvalSquareBit(rank * 8 + file);
			if (file > 0)
				adjacentMask |= EvalSquareBit(rank * 8 + file - 1);
			if (file < 7)
				adjacentMask |= EvalSquareBit(rank * 8 + file + 1);
		}

		FileMask[file] = fileMask;
		AdjacentFileMask[file] = adjacentMask;
	}
}

static void CalculatePassedPawnMasks(void)
{
	for (int side = 0; side < 2; side++)
	{
		for (int square = 0; square < 64; square++)
		{
			int file = square & 7;
			int rank = square >> 3;
			U64 mask = 0ULL;

			if (side == 0)
			{
				for (int r = rank + 1; r < 8; r++)
				{
					for (int f = file - 1; f <= file + 1; f++)
					{
						if (f >= 0 && f < 8)
							mask |= EvalSquareBit(r * 8 + f);
					}
				}
			}
			else
			{
				for (int r = rank - 1; r >= 0; r--)
				{
					for (int f = file - 1; f <= file + 1; f++)
					{
						if (f >= 0 && f < 8)
							mask |= EvalSquareBit(r * 8 + f);
					}
				}
			}

			PassedPawnMask[side][square] = mask;
		}
	}
}

static void CalculatePawnProtectorsMasks(void)
{
	for (int side = 0; side < 2; side++)
	{
		for (int square = 0; square < 64; square++)
		{
			int file = square & 7;
			int rank = square >> 3;
			U64 mask = 0ULL;

			if (side == 0)
			{
				if (rank > 0)
				{
					if (file > 0)
						mask |= EvalSquareBit(square - 9);
					if (file < 7)
						mask |= EvalSquareBit(square - 7);
				}
			}
			else
			{
				if (rank < 7)
				{
					if (file > 0)
						mask |= EvalSquareBit(square + 7);
					if (file < 7)
						mask |= EvalSquareBit(square + 9);
				}
			}

			PawnProtectorsMask[side][square] = mask;
		}
	}
}

static void CalculateKingShieldMasks(void)
{
	for (int side = 0; side < 2; side++)
	{
		for (int square = 0; square < 64; square++)
		{
			U64 mask = 0ULL;

			if (side == 1)
			{
				if (square >= 56 && square != 59 && square != 60)
				{
					if (square != 63)
						mask |= EvalSquareBit(square - 7);
					mask |= EvalSquareBit(square - 8);
					if (square != 56)
						mask |= EvalSquareBit(square - 9);
				}
			}
			else
			{
				if (square <= 7 && square != 3 && square != 4)
				{
					if (square != 7)
						mask |= EvalSquareBit(square + 9);
					mask |= EvalSquareBit(square + 8);
					if (square != 0)
						mask |= EvalSquareBit(square + 7);
				}
			}

			KingShieldMask[side][square] = mask;
		}
	}
}

short OpenRookFile(int square, Game* game, PieceType rook) {
	const AllPieceBitboards *bb = &game->Bitboards;
	int file = square & 7;
	int color01 = (rook & (BLACK | WHITE)) >> 4;
	U64 fileMask = FileMask[file];
	bool hasOwnPawn = (GetSidePawns(bb, color01) & fileMask) != 0ULL;
	bool hasOppPawn = (GetSidePawns(bb, !color01) & fileMask) != 0ULL;

	if (!hasOwnPawn && !hasOppPawn)
		return OPEN_ROOK_FILE;

	if (!hasOwnPawn)
		return OPEN_ROOK_FILE - SEMI_OPEN_FILE;

	return 0;
}

short DoublePawns(int square, Game* game, PieceType pawn) {
	const AllPieceBitboards *bb = &game->Bitboards;
	int color01 = (pawn & 24) >> 4;
	return (GetSidePawns(bb, color01) & SquareToBit(square + Behind[color01])) != 0ULL ? DOUBLE_PAWN : 0;
}

bool IsDraw(Game* game) {
	//This draw can happen early also, but cheating for performance reasons.
	/*if (game->PositionHistoryLength < 20)
		return false;*/
	// However this optimization did not give measurable improvment

	int start = game->PositionHistoryLength - 5;
	//Only checking back some moves. Possible to miss repetions but must be quite rare.
	int end = max(0, game->PositionHistoryLength - 30);
	for (int i = start; i > end; i -= 2)
	{
		if (game->Hash == game->PositionHistory[i]) //Simplyfying to 1 fold. Should not by an disadvantage.
			return true;
	}

	return game->FiftyMoveRuleCount >= 100;

	// material draw is calculated in GetEval function
}

//When king is castled at back rank, penalty for missing pawns.
short KingExposed(int square, Game* game) {
	const AllPieceBitboards *bb = &game->Bitboards;
	int color01 = (game->Squares[square] & 24) >> 4;
	U64 shieldMask = KingShieldMask[color01][square];
	U64 ownPawns = GetSidePawns(bb, color01);
	return (short)(popcount(shieldMask & ~ownPawns) * KING_EXPOSED);
}

static const int PawnPassedPoints[2][8] = {
	{ 0, 5, 10, 20, 35, 60, 100, 200 },
	{ 200, 100, 60, 35, 20, 10, 5, 0 },
};
//No opponent pawn on files left right and infront
short PassedPawn(int square, Game* game) {
	const AllPieceBitboards *bb = &game->Bitboards;
	int color01 = (game->Squares[square] & 24) >> 4;
	U64 opponentPawns = GetSidePawns(bb, !color01);
	short score = GetPassedPawnBaseScore(square, color01, opponentPawns);
	if (score == 0)
		return 0;
	return score + GetPassedPawnFreePathBonus(square, color01, bb, opponentPawns);
}

void CalculatePawnProtection() {
	for (int side = 0; side < 2; side++)
	{
		for (int square = 0; square < 64; square++)
		{
			int file = square % 8;
			//special case for file a and h (a and h)
			if (side == 0) { //white
				if (square < 8)
					PawnProtectionSquares[side][square][0] = 0; // has no protecting pawns
				else if (file == 0)
				{
					PawnProtectionSquares[side][square][0] = 1; //one protecting pawn on file a
					PawnProtectionSquares[side][square][1] = square - 7;
				}
				else if (file == 7)
				{
					PawnProtectionSquares[side][square][0] = 1; //one protecting pawn on file h
					PawnProtectionSquares[side][square][1] = square - 9;
				}
				else {
					PawnProtectionSquares[side][square][0] = 2; //two protecting pawns on the rest
					PawnProtectionSquares[side][square][1] = square - 7;
					PawnProtectionSquares[side][square][2] = square - 9;
				}

			}
			else { // BLACK
				if (square >= 56)
					PawnProtectionSquares[side][square][0] = 0; // has no protecting pawns
				else if (file == 0)
				{
					PawnProtectionSquares[side][square][0] = 1; //one protecting pawn on file a
					PawnProtectionSquares[side][square][1] = square + 9;
				}
				else if (file == 7)
				{
					PawnProtectionSquares[side][square][0] = 1; //one protecting pawn on file h
					PawnProtectionSquares[side][square][1] = square + 7;
				}
				else {
					PawnProtectionSquares[side][square][0] = 2; //two protecting pawns on the rest
					PawnProtectionSquares[side][square][1] = square + 9;
					PawnProtectionSquares[side][square][2] = square + 7;
				}
			}
		}
	}
}

void CalculatePatterns() {
	CalculateFileMasks();
	CalculatePassedPawnMasks();
	CalculatePawnProtectorsMasks();
	CalculateKingShieldMasks();
	CalculatePawnProtection();
}

short ProtectedByPawn(int square, Game* game) {
	const AllPieceBitboards *bb = &game->Bitboards;
	int color01 = (game->Squares[square] & 24) >> 4;
	return (short)(popcount(GetSidePawns(bb, color01) & PawnProtectorsMask[color01][square]) * PAWN_PROTECT);
}

static const int PiecePhase[7] = { 0, 1, 2, 4, 0, 1, 0 };
static const int MaxGamePhase = 24;
static const short TempoBonus = 8;

static int GetGamePhase(Game* game) {
	const AllPieceBitboards *bb = &game->Bitboards;
	int phase = 0;

	phase += popcount(bb->Knights.AllKnights) * PiecePhase[KNIGHT];
	phase += popcount(bb->Bishops.AllBishops) * PiecePhase[BISHOP];
	phase += popcount(bb->Rooks.AllRooks) * PiecePhase[ROOK];
	phase += popcount(bb->Queens.AllQueens) * PiecePhase[QUEEN];

	return min(MaxGamePhase, phase);
}

short GetEval(Game* game) {
	const AllPieceBitboards *bb = &game->Bitboards;

	int score = game->Material[0] + game->Material[1];
	short posScore = 0;
	//int mobil = 0;
	int neg = -1;
	int opening = 0;
	if (game->PositionHistoryLength < 12) {
		opening = 1;
	}
	int gamePhase = GetGamePhase(game);
	short pawnScore[2] = { 0, 0 };
	uchar pwnCount[2] = { 0, 0 };
	GetPawnEval(bb, pawnScore, pwnCount);

	for (int s = 0; s < 2; s++)
	{
		short scr = pawnScore[s];
		uchar bishopCount = 0;
		Piece * piece = &game->Pieces[s][0];
		while (piece != NULL)
		{
			// penalty for moving a piece more than once in the opening.
			if (opening)
			{
				if (piece->MoveCount > 1)
					scr -= SAME_TWICE;
				if (piece->Type == QUEEN)
					scr -= QUEEN_EARLY;
			}

			int i = piece->SquareIndex;
			PieceType pieceType = piece->Type;
			PieceType color = pieceType & (BLACK | WHITE);
			PieceType pt = pieceType & 7;
			
			if (pt >= 0 && pt < 7) {
                posScore += PositionValueMatrix[pt][s][i];
            }
			
			switch (pt)
			{
			case ROOK:
			{
				scr += OpenRookFile(i, game, pieceType);
				//mobil += piece.Mobility;
			}
			break;
			case BISHOP:
			case KNIGHT: {
				bishopCount += (pt == BISHOP);
				scr += ProtectedByPawn(i, game);
			}
					   break;
			case PAWN: {
			}
					 break;
			case KING: {
				int color01 = color >> 4;
				int kingSafety = KingPositionValueMatrix[0][color01][i];
				int kingActivity = KingPositionValueMatrix[1][color01][i];
				posScore += (short)((kingSafety * gamePhase + kingActivity * (MaxGamePhase - gamePhase)) / MaxGamePhase);
				scr -= (short)((KingExposed(i, game) * gamePhase) / MaxGamePhase);
			}
					 break;
			default:
				break;
			}

			piece = piece->Next;
		}

		scr += BISHOP_PAIR * (bishopCount == 2);
		score += (neg * scr);
		neg += 2; // -1 --> 1 // White then black
	}

	// the lead is more worth when there is less material on the board
	// lead of 200 with material 2000 (two queen) -> 25p
	// supposed to make leader prefer chaning down.

	/*short materialLead = game->Material[1] - game->Material[0];
	if (materialLead)
		return score + posScore + (float)(score * 50) / (float)materialLead;*/

		//insuficient material check
	int eval = score + posScore;
	bool whiteNoPawns = pwnCount[0] == 0;
	bool blackNoPawns = pwnCount[1] == 0;
	bool whiteInsufficient = game->Material[0] >= -MATERIAL_N_N;
	bool blackInsufficient = game->Material[1] <= MATERIAL_N_N;

	if (whiteNoPawns && blackNoPawns && whiteInsufficient && blackInsufficient)
		return 0;

	if (whiteNoPawns && whiteInsufficient)
		return max(0, eval);

	if (blackNoPawns && blackInsufficient)
		return min(0, eval);

	if (game->Side == BLACK)
		eval += TempoBonus;
	else
		eval -= TempoBonus;

	return eval;
}

short TotalMaterial(Game* game) {
	return game->Material[0] + game->Material[1];
}

void AdjustPositionImportance()
{
	for (int i = 1; i < 7; i++)
	{
		for (int s = 0; s < 64; s++)
		{
			PositionValueMatrix[i][0][s] = PositionValueMatrix[i][0][s] / 3;
			PositionValueMatrix[i][1][s] = PositionValueMatrix[i][1][s] / 3;
		}
	}

	for (int i = 0; i < 64; i++)
	{
		KingPositionValueMatrix[0][0][i] = KingPositionValueMatrix[0][0][i] / 3;
		KingPositionValueMatrix[1][0][i] = KingPositionValueMatrix[1][0][i] / 3;

		KingPositionValueMatrix[0][1][i] = KingPositionValueMatrix[0][1][i] / 3;
		KingPositionValueMatrix[1][1][i] = KingPositionValueMatrix[1][1][i] / 3;
	}
}

void SwitchSignOfWhitePositionValue()
{
	for (int i = 1; i < 7; i++)
	{
		for (int s = 0; s < 64; s++)
		{
			PositionValueMatrix[i][0][s] = -PositionValueMatrix[i][0][s];
		}
	}

	for (int i = 0; i < 64; i++)
	{
		KingPositionValueMatrix[0][0][i] = -KingPositionValueMatrix[0][0][i];
		KingPositionValueMatrix[1][0][i] = -KingPositionValueMatrix[1][0][i];
	}
}