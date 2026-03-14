#include "bitboards.h"

static AllPieceBitboards BuildAllPieceBitboards(const Game *game)
{
	U64 bb[7][2] = {0};
	AllPieceBitboards bitboards = {0};
	bitboards.WhitePieces = 0ULL;
	bitboards.BlackPieces = 0ULL;
	bitboards.Occupied = 0ULL;

	for (int square = 0; square < 64; square++)
	{
		PieceType piece = game->Squares[square];
		if (piece == NOPIECE)
			continue;

		int color = piece >> 4;
		if (color > 1)
			continue;

		int pieceType = piece & 7;
		if (pieceType < BISHOP || pieceType > KING)
			continue;

		U64 bit = SquareToBit(square);
		if (color == 0)
			bitboards.WhitePieces |= bit;
		else
			bitboards.BlackPieces |= bit;
		bb[pieceType][color] |= bit;
	}

	bitboards.Pawns.WhitePawns = bb[PAWN][0];
	bitboards.Pawns.BlackPawns = bb[PAWN][1];
	bitboards.Pawns.AllPawns = bb[PAWN][0] | bb[PAWN][1];

	bitboards.Knights.WhiteKnights = bb[KNIGHT][0];
	bitboards.Knights.BlackKnights = bb[KNIGHT][1];
	bitboards.Knights.AllKnights = bb[KNIGHT][0] | bb[KNIGHT][1];

	bitboards.Bishops.WhiteBishops = bb[BISHOP][0];
	bitboards.Bishops.BlackBishops = bb[BISHOP][1];
	bitboards.Bishops.AllBishops = bb[BISHOP][0] | bb[BISHOP][1];

	bitboards.Rooks.WhiteRooks = bb[ROOK][0];
	bitboards.Rooks.BlackRooks = bb[ROOK][1];
	bitboards.Rooks.AllRooks = bb[ROOK][0] | bb[ROOK][1];

	bitboards.Queens.WhiteQueens = bb[QUEEN][0];
	bitboards.Queens.BlackQueens = bb[QUEEN][1];
	bitboards.Queens.AllQueens = bb[QUEEN][0] | bb[QUEEN][1];

	bitboards.Kings.WhiteKing = bb[KING][0];
	bitboards.Kings.BlackKing = bb[KING][1];
	bitboards.Kings.AllKings = bb[KING][0] | bb[KING][1];

	bitboards.Occupied = bitboards.WhitePieces | bitboards.BlackPieces;
	for (int pieceType = PAWN; pieceType <= KING; pieceType++)
	{
		bitboards.Matrix[pieceType][0] = bb[pieceType][0];
		bitboards.Matrix[pieceType][1] = bb[pieceType][1];
	}

	return bitboards;
}

U64 SquareToBit(int square)
{
	if (square < 0 || square > 63)
		return 0ULL;
	return 1ULL << square;
}

void SyncGameBitboards(Game *game)
{
	game->Bitboards = BuildAllPieceBitboards(game);
}

AllPieceBitboards GetAllPieceBitboards(const Game *game)
{
	return BuildAllPieceBitboards(game);
}

PawnBitboards GetPawnBitboards(const Game *game)
{
	return GetAllPieceBitboards(game).Pawns;
}

KnightBitboards GetKnightBitboards(const Game *game)
{
	return GetAllPieceBitboards(game).Knights;
}

BishopBitboards GetBishopBitboards(const Game *game)
{
	return GetAllPieceBitboards(game).Bishops;
}

RookBitboards GetRookBitboards(const Game *game)
{
	return GetAllPieceBitboards(game).Rooks;
}

QueenBitboards GetQueenBitboards(const Game *game)
{
	return GetAllPieceBitboards(game).Queens;
}

KingBitboards GetKingBitboards(const Game *game)
{
	return GetAllPieceBitboards(game).Kings;
}
