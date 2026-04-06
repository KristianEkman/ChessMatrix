#include "commons.h"
#include "evaluation.h"
#include "bitboards.h"
#include "patterns.h"
#include <stdlib.h>


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
static const short PawnPhalanxPoints[8] = { 0, 0, 4, 8, 12, 16, 0, 0 };

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

static PieceType GetPromotionPieceType(MoveInfo moveInfo)
{
	switch (moveInfo)
	{
	case PromotionQueen:
		return QUEEN;
	case PromotionRook:
		return ROOK;
	case PromotionBishop:
		return BISHOP;
	case PromotionKnight:
		return KNIGHT;
	default:
		return NOPIECE;
	}
}

static short GetPieceMaterialValue(PieceType pieceType)
{
	switch (pieceType & 7)
	{
	case BISHOP:
		return MATERIAL_B;
	case ROOK:
		return MATERIAL_R;
	case QUEEN:
		return MATERIAL_Q;
	case PAWN:
		return MATERIAL_P;
	case KNIGHT:
		return MATERIAL_N;
	default:
		return 0;
	}
}

static short GetCaptureOrderingBonus(PieceType attackerType, PieceType victimType)
{
	short victimValue = GetPieceMaterialValue(victimType);
	short attackerValue = GetPieceMaterialValue(attackerType);

	return (short)(96 + victimValue / 16 - attackerValue / 32);
}

static bool GetCastleRookMove(Move move, int side01, int *rookFrom, int *rookTo)
	{
		if (move.MoveInfo == CastleShort)
		{
			*rookFrom = side01 == 0 ? h1 : h8;
			*rookTo = side01 == 0 ? f1 : f8;
			return true;
		}

		if (move.MoveInfo == CastleLong)
		{
			*rookFrom = side01 == 0 ? a1 : a8;
			*rookTo = side01 == 0 ? d1 : d8;
			return true;
		}

		return false;
	}

short GetMoveOrderingScore(Move move, Game *game)
	{
	short moveScore = game->Material[0] + game->Material[1];
	int from = move.From;
	int to = move.To;

	PieceType capturedType = game->Squares[to];
	int capturedColor = capturedType >> 4;
	int side01 = game->Side01;

	PieceType pieceType = game->Squares[from];
	PieceType pt = pieceType & 7;
	PieceType promotionPiece = GetPromotionPieceType((MoveInfo)move.MoveInfo);
	short sideSign = side01 == 0 ? -1 : 1;

	// removing piece from square removes its position score
	moveScore -= PositionValueMatrix[capturedType & 7][capturedColor][to];
	moveScore -= PositionValueMatrix[pt][side01][from];
	moveScore += PositionValueMatrix[pt][side01][to];

	if (capturedType && move.MoveInfo != EnPassantCapture)
	{
		moveScore -= MaterialMatrix[capturedColor][capturedType & 7];
		moveScore += (short)(sideSign * GetCaptureOrderingBonus(pieceType, capturedType));
	}

	if (promotionPiece != NOPIECE)
	{
		moveScore -= PositionValueMatrix[PAWN][side01][to];
		moveScore += PositionValueMatrix[promotionPiece][side01][to];
		moveScore += MaterialMatrix[side01][promotionPiece + 6];
	}

	switch (move.MoveInfo)
	{
	case KingMove:
		moveScore += GetKingPositionScore(move, game);
		break;
	case CastleShort:
	case CastleLong:
		{
			int rookFrom = 0;
			int rookTo = 0;

			moveScore += CastlingPoints[side01];
			moveScore += GetKingPositionScore(move, game);
			if (GetCastleRookMove(move, side01, &rookFrom, &rookTo))
				moveScore += PositionValueMatrix[ROOK][side01][rookTo] - PositionValueMatrix[ROOK][side01][rookFrom];
			break;
		}
	case EnPassantCapture:
		{
			int capturedPawnSquare = to + Behind[side01];
			moveScore += MaterialMatrix[side01][PAWN]; // Adding own pawn material is same as removing opponent.
			moveScore -= PositionValueMatrix[PAWN][!side01][capturedPawnSquare];
			moveScore += (short)(sideSign * GetCaptureOrderingBonus(pieceType, PAWN));
			break;
		}
	default:
		break;
	}

	return moveScore;
}

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

static short GetPawnPhalanxScore(int square, int side01, U64 ownPawns)
{
	int file = square & 7;
	int rank = square >> 3;
	int relativeRank = side01 == 0 ? rank : 7 - rank;
	bool hasLeftMate = file > 0 && (ownPawns & SquareBitUnchecked(square - 1)) != 0ULL;
	bool hasRightMate = file < 7 && (ownPawns & SquareBitUnchecked(square + 1)) != 0ULL;

	if (!hasLeftMate && !hasRightMate)
		return 0;

	return PawnPhalanxPoints[relativeRank];
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

static U64 GetForwardFileMask(int square, int side01)
{
	int file = square & 7;
	int rank = square >> 3;

	if (side01 == 0)
	{
		int shift = (rank + 1) * 8;
		return shift < 64 ? FileMask[file] & (~0ULL << shift) : 0ULL;
	}

	{
		int shift = rank * 8;
		return shift > 0 ? FileMask[file] & ((1ULL << shift) - 1ULL) : 0ULL;
	}
}

static bool IsPureKingAndPawnEnding(const AllPieceBitboards *bb)
{
	return bb->Knights.AllKnights == 0ULL &&
		   bb->Bishops.AllBishops == 0ULL &&
		   bb->Rooks.AllRooks == 0ULL &&
		   bb->Queens.AllQueens == 0ULL;
}

static int GetChebyshevDistance(int from, int to)
{
	int fileDistance = abs((from & 7) - (to & 7));
	int rankDistance = abs((from >> 3) - (to >> 3));
	return max(fileDistance, rankDistance);
}

static short GetPassedPawnRaceBonus(int square, int side01, Game *game)
{
	const AllPieceBitboards *bb = &game->Bitboards;
	if (!IsPureKingAndPawnEnding(bb))
		return 0;

	U64 opponentPawns = GetSidePawns(bb, !side01);
	if (GetPassedPawnBaseScore(square, side01, opponentPawns) == 0)
		return 0;

	int file = square & 7;
	int rank = square >> 3;
	int promotionSquare = side01 == 0 ? 56 + file : file;
	int pawnSteps = side01 == 0 ? 7 - rank : rank;
	int effectiveSteps = max(0, pawnSteps - (game->Side01 == side01 ? 1 : 0));
	int enemyKingDistance = GetChebyshevDistance(game->KingSquares[!side01], promotionSquare);
	int raceMargin = enemyKingDistance - effectiveSteps;
	int progress = 7 - pawnSteps;
	int cappedMargin = 0;
	U64 forwardFileMask = GetForwardFileMask(square, side01);

	if ((bb->Occupied & forwardFileMask) != 0ULL)
		return 0;

	if (raceMargin <= 0)
		return 0;

	cappedMargin = min(3, raceMargin);
	return (short)(120 + progress * 30 + cappedMargin * 20);
}

short PassedPawnRaceBonus(int square, Game *game)
{
	PieceType pieceType = game->Squares[square];

	if ((pieceType & 7) != PAWN)
		return 0;

	return GetPassedPawnRaceBonus(square, pieceType >> 4, game);
}

short PawnPhalanx(int square, Game *game)
{
	PieceType pieceType = game->Squares[square];
	int side01 = pieceType >> 4;

	if ((pieceType & 7) != PAWN)
		return 0;

	return GetPawnPhalanxScore(square, side01, GetSidePawns(&game->Bitboards, side01));
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
				if (ownPawns & SquareBitUnchecked(square + Behind[side01]))
					score -= DOUBLE_PAWN;
				score += (short)(popcount(ownPawns & PawnProtectorsMask[side01][square]) * PAWN_PROTECT);
				score += GetPawnPhalanxScore(square, side01, ownPawns);
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
		return SEMI_OPEN_FILE;

	return 0;
}

static short GetRookBehindPassedPawnScore(int rookSquare, int side01, const AllPieceBitboards *bb)
{
	int file = rookSquare & 7;
	int rookRank = rookSquare >> 3;
	U64 ownPawns = GetSidePawns(bb, side01);
	U64 opponentPawns = GetSidePawns(bb, !side01);
	U64 filePawns = ownPawns & FileMask[file];

	while (filePawns)
	{
		int pawnSquare = pop_lsb(&filePawns);
		if ((opponentPawns & PassedPawnMask[side01][pawnSquare]) == 0ULL)
		{
			int pawnRank = pawnSquare >> 3;
			bool rookBehind = side01 == 0 ? rookRank < pawnRank : rookRank > pawnRank;
			if (rookBehind)
				return 15;
		}
	}

	return 0;
}

short RookBehindPassedPawn(int square, Game *game)
{
	PieceType pieceType = game->Squares[square];
	int side01 = pieceType >> 4;

	if ((pieceType & 7) != ROOK)
		return 0;

	return GetRookBehindPassedPawnScore(square, side01, &game->Bitboards);
}

static short GetProtectedByPawnScore(U64 ownPawns, int side01, int square)
{
	return (short)(popcount(ownPawns & PawnProtectorsMask[side01][square]) * PAWN_PROTECT);
}

static short GetBishopMobilityScore(int square, U64 ownPieces, U64 occupied)
{
	short mobility = 0;
	const char (*rays)[8] = PieceTypeSquareRaysPatterns[0][square];
	int raysCount = rays[0][0];

	for (int r = 1; r <= raysCount; r++)
	{
		int rayLength = rays[r][0];
		for (int rr = 1; rr <= rayLength; rr++)
		{
			int targetSquare = rays[r][rr];
			U64 targetBit = SquareBitUnchecked(targetSquare);

			if (ownPieces & targetBit)
				break;

			mobility++;
			if (occupied & targetBit)
				break;
		}
	}

	return (short)(mobility * BISHOP_MOBILITY);
}

short BishopMobility(int square, Game* game)
{
	const AllPieceBitboards *bb = &game->Bitboards;
	PieceType bishop = game->Squares[square];
	int color01 = (bishop & (BLACK | WHITE)) >> 4;

	if ((bishop & 7) != BISHOP)
		return 0;

	return GetBishopMobilityScore(square, color01 == 0 ? bb->WhitePieces : bb->BlackPieces, bb->Occupied);
}

short DoublePawns(int square, Game* game, PieceType pawn) {
	const AllPieceBitboards *bb = &game->Bitboards;
	int color01 = (pawn & 24) >> 4;
	return (GetSidePawns(bb, color01) & SquareBitUnchecked(square + Behind[color01])) != 0ULL ? DOUBLE_PAWN : 0;
}

bool IsDraw(Game* game) {
	int start = game->PositionHistoryLength - 3;
	int end = max(0, game->PositionHistoryLength - game->FiftyMoveRuleCount - 1);
	for (int i = start; i >= end; i -= 2)
	{
		if (game->Hash == game->PositionHistory[i])
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
	return GetProtectedByPawnScore(GetSidePawns(bb, color01), color01, square);
}

static short GetEndgameKingPawnTropismScore(int square, int side01, Game *game)
{
	const AllPieceBitboards *bb = &game->Bitboards;
	U64 enemyPawns = GetSidePawns(bb, !side01);
	short score = 0;
	const int maxGamePhase = 24;
	int gamePhase = GetGamePhase(game);

	if (bb->Queens.AllQueens != 0ULL || bb->Rooks.AllRooks != 0ULL)
		return 0;

	while (enemyPawns)
	{
		int pawnSquare = pop_lsb(&enemyPawns);
		int distance = GetChebyshevDistance(square, pawnSquare);
		if (distance < 5)
			score += (short)((5 - distance) * 2);
	}

	return (short)((score * (maxGamePhase - gamePhase)) / maxGamePhase);
}

short EndgameKingPawnTropism(int square, Game *game)
{
	PieceType pieceType = game->Squares[square];

	if ((pieceType & 7) != KING)
		return 0;

	return GetEndgameKingPawnTropismScore(square, pieceType >> 4, game);
}

static const int PiecePhase[7] = { 0, 1, 2, 4, 0, 1, 0 };
static const int MaxGamePhase = 24;
static const int OpeningPhaseThreshold = 20;
static const short TempoBonus = 8;

static bool IsOpeningPhase(int gamePhase)
{
	return gamePhase >= OpeningPhaseThreshold;
}

int GetGamePhase(Game* game) {
	const AllPieceBitboards *bb = &game->Bitboards;
	int phase = 0;

	phase += popcount(bb->Knights.AllKnights) * PiecePhase[KNIGHT];
	phase += popcount(bb->Bishops.AllBishops) * PiecePhase[BISHOP];
	phase += popcount(bb->Rooks.AllRooks) * PiecePhase[ROOK];
	phase += popcount(bb->Queens.AllQueens) * PiecePhase[QUEEN];

	return min(MaxGamePhase, phase);
}

short GetKingPositionScore(Move move, Game *game)
{
	int gamePhase = GetGamePhase(game);
	int middleGameDelta = KingPositionValueMatrix[0][game->Side01][move.To] - KingPositionValueMatrix[0][game->Side01][move.From];
	int endGameDelta = KingPositionValueMatrix[1][game->Side01][move.To] - KingPositionValueMatrix[1][game->Side01][move.From];

	return (short)((middleGameDelta * gamePhase + endGameDelta * (MaxGamePhase - gamePhase)) / MaxGamePhase);
}

static short GetSimplificationBonusScore(int materialBalance, const AllPieceBitboards *bb)
{
	if (materialBalance > -MATERIAL_P && materialBalance < MATERIAL_P)
		return 0;

	int nonPawnPieces = popcount(bb->Knights.AllKnights) + popcount(bb->Bishops.AllBishops) +
	                    popcount(bb->Rooks.AllRooks) + popcount(bb->Queens.AllQueens);
	int tradedPieces = 14 - nonPawnPieces;

	if (tradedPieces <= 0)
		return 0;

	short bonus = (short)(tradedPieces * 4);
	return materialBalance > 0 ? bonus : (short)-bonus;
}

short SimplificationBonus(Game *game)
{
	int materialBalance = game->Material[0] + game->Material[1];
	return GetSimplificationBonusScore(materialBalance, &game->Bitboards);
}

short GetEval(Game* game) {
	const AllPieceBitboards *bb = &game->Bitboards;

	int score = game->Material[0] + game->Material[1];
	short posScore = 0;
	//int mobil = 0;
	int neg = -1;
	int gamePhase = GetGamePhase(game);
	bool opening = IsOpeningPhase(gamePhase);
	short pawnScore[2] = { 0, 0 };
	uchar pwnCount[2] = { 0, 0 };
	GetPawnEval(bb, pawnScore, pwnCount);

	for (int s = 0; s < 2; s++)
	{
		short scr = pawnScore[s];
		uchar bishopCount = 0;
		U64 ownPawns = GetSidePawns(bb, s);
		U64 ownPieces = s == 0 ? bb->WhitePieces : bb->BlackPieces;
		Piece * piece = &game->Pieces[s][0];
		while (piece != NULL)
		{
			PieceType pieceType = piece->Type;
			PieceType color = pieceType & (BLACK | WHITE);
			PieceType pt = pieceType & 7;

			// penalty for moving a piece more than once in the opening.
			if (opening)
			{
				if (piece->MoveCount > 1)
					scr -= SAME_TWICE;
				if (pt == QUEEN && piece->MoveCount > 0)
					scr -= QUEEN_EARLY;
			}

			int i = piece->SquareIndex;
			
			if (pt >= 0 && pt < 7) {
                posScore += PositionValueMatrix[pt][s][i];
            }
			
			switch (pt)
			{
			case ROOK:
			{
				scr += OpenRookFile(i, game, pieceType);
				scr += GetRookBehindPassedPawnScore(i, s, bb);
				//mobil += piece.Mobility;
			}
			break;
			case BISHOP:
			{
				bishopCount++;
				scr += GetProtectedByPawnScore(ownPawns, s, i);
				scr += GetBishopMobilityScore(i, ownPieces, bb->Occupied);
			}
					   break;
			case KNIGHT:
			{
				scr += GetProtectedByPawnScore(ownPawns, s, i);
			}
					   break;
			case PAWN: {
				scr += GetPassedPawnRaceBonus(i, s, game);
			}
					 break;
			case KING: {
				int color01 = color >> 4;
				int kingSafety = KingPositionValueMatrix[0][color01][i];
				int kingActivity = KingPositionValueMatrix[1][color01][i];
				posScore += (short)((kingSafety * gamePhase + kingActivity * (MaxGamePhase - gamePhase)) / MaxGamePhase);
				scr += GetEndgameKingPawnTropismScore(i, color01, game);
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
	eval += GetSimplificationBonusScore(game->Material[0] + game->Material[1], bb);
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