#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <Windows.h>

#include "basic_structs.h"
#include "patterns.h"
#include "main.h"
#include "utils.h"
#include "tests.h"
#include "evaluation.h"
#include "sort.h"

const int MOVESIZE = sizeof(Move);
Game mainGame;
Game threadGames[SEARCH_THREADS];

int _behind[] = { -8, 8 };
int SearchedLeafs = 0;

int From(Move move) {
	return (move >> 16) & FromMask;
}

int To(Move move) {
	return (move >> 22) & ToMask;
}

MoveInfo Info(move) {
	return (move >> 28);
}

void InitPiece(int file, int rank, enum PieceType type, enum Color color) {
	mainGame.Squares[rank * 8 + file] = type | color;
}

void InitGame() {
	for (int i = 0; i < 64; i++)
		mainGame.Squares[i] = NOPIECE;

	InitPiece(0, 0, ROOK, WHITE);
	InitPiece(1, 0, KNIGHT, WHITE);
	InitPiece(2, 0, BISHOP, WHITE);
	InitPiece(3, 0, QUEEN, WHITE);
	InitPiece(4, 0, KING, WHITE);
	mainGame.KingSquares[0] = 4;
	InitPiece(5, 0, BISHOP, WHITE);
	InitPiece(6, 0, KNIGHT, WHITE);
	InitPiece(7, 0, ROOK, WHITE);

	for (int i = 0; i < 8; i++)
		InitPiece(i, 1, PAWN, WHITE);

	InitPiece(0, 7, ROOK, BLACK);
	InitPiece(1, 7, KNIGHT, BLACK);
	InitPiece(2, 7, BISHOP, BLACK);
	InitPiece(3, 7, QUEEN, BLACK);
	InitPiece(4, 7, KING, BLACK);
	mainGame.KingSquares[1] = 60;
	InitPiece(5, 7, BISHOP, BLACK);
	InitPiece(6, 7, KNIGHT, BLACK);
	InitPiece(7, 7, ROOK, BLACK);

	for (int i = 0; i < 8; i++)
		InitPiece(i, 6, PAWN, BLACK);
	mainGame.Side = WHITE;

	mainGame.State = WhiteCanCastleLong | WhiteCanCastleShort | BlackCanCastleLong | BlackCanCastleShort;
	mainGame.Material[0] = 0;
	mainGame.Material[1] = 0;

}

char PieceChar(PieceType pieceType) {
	PieceType color = pieceType & (BLACK | WHITE);
	PieceType pt = pieceType & 7;
	switch (pt)
	{
	case PAWN:
		return color == WHITE ? 'P' : 'p';
	case ROOK:
		return color == WHITE ? 'R' : 'r';
	case KNIGHT:
		return color == WHITE ? 'N' : 'n';
	case BISHOP:
		return color == WHITE ? 'B' : 'b';
	case QUEEN:
		return color == WHITE ? 'Q' : 'q';
	case KING:
		return color == WHITE ? 'K' : 'k';
	default:
		return ' ';
	}
}

void PrintGame() {
	printf("  ---------------------------------\n");

	for (int r = 8 - 1; r >= 0; r--)
	{
		printf("%d ", r + 1);
		for (int f = 0; f < 8; f++)
		{
			PieceType piece = mainGame.Squares[r * 8 + f];
			char c = PieceChar(piece);
			printf("| %c ", c);
		}
		printf("|\n  ---------------------------------\n");
	}
	printf("    a   b   c   d   e   f   g   h  \n");
}

void KingPositionScore(int f, int t, Game * game) {
	//aproximation that endgame starts att 1900.
	int endGame = game->Material[1] - game->Material[0] < 1900 ? 1 : 0;
	game->PositionScore += KingPositionValueMatrix[endGame][game->Side >> 4][t];
	game->PositionScore -= KingPositionValueMatrix[endGame][game->Side >> 4][f];
}

void MakeMove(Move move, Game * game) {
	int f = From(move);
	int t = To(move);

	PieceType captType = game->Squares[t];
	int captColor = captType >> 4;
	int side01 = game->Side >> 4;

	//Capturing
	game->Material[captColor] -= MaterialMatrix[captColor][captType & 7];

	//removing piece from square removes its position score
	game->PositionScore -= PositionValueMatrix[captType & 7][captColor][t];

	PieceType pieceType = game->Squares[f];

	char pt = pieceType & 7;
	game->PositionScore -= PositionValueMatrix[pt][side01][f];
	game->PositionScore += PositionValueMatrix[pt][side01][t];

	game->Squares[t] = game->Squares[f];
	game->Squares[f] = NOPIECE;

	//resetting en passant every move
	game->State &= ~15;
	MoveInfo info = Info(move);
	switch (info)
	{
	case PromotionQueen:
		game->Squares[t] = QUEEN | game->Side;
		game->Material[side01] += MaterialMatrix[side01][QUEEN + 6];
		game->PositionScore += PositionValueMatrix[QUEEN][side01][t];
		break;
	case PromotionRook:
		game->Squares[t] = ROOK | game->Side;
		game->Material[side01] += MaterialMatrix[side01][ROOK + 6];
		game->PositionScore += PositionValueMatrix[ROOK][side01][t];
		break;
	case PromotionBishop:
		game->Squares[t] = BISHOP | game->Side;
		game->Material[side01] += MaterialMatrix[side01][BISHOP + 6];
		game->PositionScore += PositionValueMatrix[BISHOP][side01][t];
		break;
	case PromotionKnight:
		game->Squares[t] = KNIGHT | game->Side;
		game->Material[side01] += MaterialMatrix[side01][KNIGHT + 6];
		game->PositionScore += PositionValueMatrix[KNIGHT][side01][t];
		break;
	case KingMove:
		game->KingSquares[side01] = t;
		KingPositionScore(f, t, game);
		break;
	case RookMove:
		switch (f)
		{
		case 0:
			game->State &= ~WhiteCanCastleLong;
			break;
		case 7:
			game->State &= ~WhiteCanCastleShort;
			break;
		case 56:
			game->State &= ~BlackCanCastleLong;
			break;
		case 63:
			game->State &= ~BlackCanCastleShort;
			break;
		default:
			break;
		}
		break;
	case CastleShort:
	{
		game->KingSquares[side01] = t;
		char rookFr = 7 + CastlesOffset[side01];
		char rookTo = 5 + CastlesOffset[side01];
		PieceType rook = ROOK | game->Side;
		game->Squares[rookFr] = NOPIECE;
		game->Squares[rookTo] = rook;

		game->PositionScore -= PositionValueMatrix[ROOK][side01][rookFr];
		game->PositionScore += PositionValueMatrix[ROOK][side01][rookTo];
		KingPositionScore(f, t, game);
	}
	break;
	case CastleLong:
	{
		game->KingSquares[side01] = t;
		char rookFr = CastlesOffset[side01];
		char rookTo = 3 + CastlesOffset[side01];
		PieceType rook = ROOK | game->Side;
		game->Squares[rookFr] = NOPIECE;
		game->Squares[rookTo] = ROOK | game->Side;

		game->PositionScore -= PositionValueMatrix[ROOK][side01][rookFr];
		game->PositionScore += PositionValueMatrix[ROOK][side01][rookTo];
		KingPositionScore(f, t, game);
	}
	break;
	case EnPassant:
		game->State |= ((f & 7) + 1); //Sets the file. a to h file is 1 to 8.
		break;
	case EnPassantCapture:
	{
		char behind = t + _behind[side01];
		game->Squares[behind] = NOPIECE;
		game->Material[side01] += MaterialMatrix[side01][PAWN];
		game->PositionScore -= PositionValueMatrix[PAWN][captColor][behind];
	}
	break;
	default:
		break;
	}
	game->Side ^= 24;
}

void UnMakeMove(Move move, PieceType capture, GameState prevGameState, int prevPositionScore, Game * game) {

	game->Material[capture >> 4] += MaterialMatrix[capture >> 4][capture & 7];
	int f = From(move);
	int t = To(move);

	game->Squares[f] = game->Squares[t];
	game->Squares[t] = capture;
	int otherSide = game->Side ^ 24;
	int otherSide01 = otherSide >> 4;
	switch (Info(move))
	{
	case PromotionQueen:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][QUEEN + 6];
		game->Squares[f] = PAWN | otherSide;
		break;
	case PromotionRook:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][ROOK + 6];
		game->Squares[f] = PAWN | otherSide;
		break;
	case PromotionBishop:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][BISHOP + 6];
		game->Squares[f] = PAWN | otherSide;
		break;
	case PromotionKnight:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][KNIGHT + 6];
		game->Squares[f] = PAWN | otherSide;
		break;
	case KingMove:
		game->KingSquares[otherSide01] = f;
		break;
	case CastleShort:
		game->KingSquares[otherSide01] = f;
		game->Squares[5 + CastlesOffset[otherSide01]] = NOPIECE;
		game->Squares[7 + CastlesOffset[otherSide01]] = ROOK | otherSide;
		break;
	case CastleLong:
		game->KingSquares[otherSide01] = f;
		game->Squares[3 + CastlesOffset[otherSide01]] = NOPIECE;
		game->Squares[0 + CastlesOffset[otherSide01]] = ROOK | otherSide;
		break;
	case EnPassantCapture:
		game->Squares[t + _behind[otherSide01]] = PAWN | game->Side;
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][PAWN];
		break;
	default:
		break;
	}
	game->State = prevGameState;
	game->PositionScore = prevPositionScore;
	game->Side ^= 24;
}


bool SquareAttacked(int square, char attackedBy, Game * game) {
	for (int i = 0; i < 64; i++)
	{
		PieceType pieceType = game->Squares[i];
		PieceType color = pieceType & (BLACK | WHITE);

		if (color != attackedBy)
			continue;
		PieceType pt = pieceType & 7;
		switch (pt)
		{
		case PAWN:
		{
			int captPat = PawnCapturePattern[attackedBy >> 4];
			int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
			for (int pc = 1; pc <= pawnCapPatLength; pc++)
			{
				int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
				if (toSquare == square)
					return true;
			}
			break;
		}
		case KNIGHT:
		{
			int length = PieceTypeSquarePatterns[0][i][0];
			for (int p = 1; p <= length; p++)
			{
				int toSquare = PieceTypeSquarePatterns[0][i][p];
				if (toSquare == square)
					return true;
			}
			break;
		}
		case KING:
		{
			int length = PieceTypeSquarePatterns[1][i][0];
			for (int p = 1; p <= length; p++)
			{
				int toSquare = PieceTypeSquarePatterns[1][i][p];
				if (toSquare == square)
					return true;
			}
			break;
		}
		default:
		{
			int pat = pt - 1;
			char raysCount = PieceTypeSquareRaysPatterns[pat][i][0][0];
			for (int r = 1; r <= raysCount; r++)
			{
				int rayLength = PieceTypeSquareRaysPatterns[pat][i][r][0];
				for (int rr = 1; rr <= rayLength; rr++)
				{
					int toSquare = PieceTypeSquareRaysPatterns[pat][i][r][rr];
					if (toSquare == square)
						return true;
					if (game->Squares[toSquare] > NOPIECE)
						break;
				}
			}
			break;
		}
		}

	}
	return false;
}

void SortMoves(Move * moves, int moveCount, Game * game) {
	if (game->Side == WHITE)
		QuickSort(moves, 0, moveCount - 1);
	else
		QuickSortDescending(moves, 0, moveCount - 1);
}

void CreateMove(int fromSquare, int toSquare, MoveInfo moveInfo, Game * game) {
	PieceType capture = game->Squares[toSquare];
	GameState prevGameState = game->State;
	Move move = 0;
	move |= ( fromSquare << 16 );
	move |= (toSquare << 22);
	move |= (moveInfo << 28);
	int prevPosScore = game->PositionScore;
	MakeMove(move, game);
	move |= (GetScore(game) & 0xFFFF);
	int  kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
	bool legal = !SquareAttacked(kingSquare, game->Side, game);
	if (legal)
		game->MovesBuffer[game->MovesBufferLength++] = move;

	UnMakeMove(move, capture, prevGameState, prevPosScore, game);
}

void CreateMoves(Game * game) {
	game->MovesBufferLength = 0;
	for (int i = 0; i < 64; i++)
	{
		PieceType pieceType = game->Squares[i];
		if (pieceType & game->Side) {
			PieceType pt = pieceType & 7;
			switch (pt)
			{
			case PAWN:
			{
				int pat = PawnPattern[game->Side >> 4];
				int pawnPatLength = PieceTypeSquarePatterns[pat][i][0];
				for (int pp = 1; pp <= pawnPatLength; pp++)
				{
					int toSquare = PieceTypeSquarePatterns[pat][i][pp];
					if (game->Squares[toSquare] != NOPIECE)
						break;
					if (toSquare < 8 || toSquare > 55) {
						CreateMove(i, toSquare, PromotionQueen, game);
						CreateMove(i, toSquare, PromotionRook, game);
						CreateMove(i, toSquare, PromotionBishop, game);
						CreateMove(i, toSquare, PromotionKnight, game);
					}
					else if (pp == 2) {
						CreateMove(i, toSquare, EnPassant, game);
					}
					else {
						CreateMove(i, toSquare, PlainMove, game);
					}
				}

				int captPat = PawnCapturePattern[game->Side >> 4];
				int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
				for (int pc = 1; pc <= pawnCapPatLength; pc++)
				{
					int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
					//Must be a piece of opposite color.
					if (game->Squares[toSquare] & (game->Side ^ 24))
					{
						if (toSquare < 8 || toSquare > 55) {
							CreateMove(i, toSquare, PromotionQueen, game);
							CreateMove(i, toSquare, PromotionRook, game);
							CreateMove(i, toSquare, PromotionBishop, game);
							CreateMove(i, toSquare, PromotionKnight, game);
						}
						else {
							CreateMove(i, toSquare, PlainMove, game);
						}
					}
					else {
						int enpFile = (game->State & 15) - 1;
						if (enpFile > -1) {
							int toFile = toSquare & 7;
							int toRank = toSquare >> 3;
							if (toFile == enpFile && toRank == EnpassantRankPattern[game->Side >> 4])
								CreateMove(i, toSquare, EnPassantCapture, game);
						}
					}
				}
				break;
			}
			case KNIGHT:
			{
				int length = PieceTypeSquarePatterns[0][i][0];
				for (int p = 1; p <= length; p++)
				{
					int toSquare = PieceTypeSquarePatterns[0][i][p];
					if (!(game->Squares[toSquare] & game->Side)) {
						CreateMove(i, toSquare, 0, game);
					}
				}
				break;
			}
			case KING:
			{
				int length = PieceTypeSquarePatterns[1][i][0];
				for (int p = 1; p <= length; p++)
				{
					int toSquare = PieceTypeSquarePatterns[1][i][p];
					if (!(game->Squares[toSquare] & game->Side)) {
						CreateMove(i, toSquare, KingMove, game);
					}
				}

				int castleBlackOffset = CastlesOffset[game->Side >> 4];
				if (i == castleBlackOffset + 4) { //King on origin pos
					if ((game->Side & WHITE && game->State & WhiteCanCastleShort) || (game->Side & BLACK && game->State & BlackCanCastleShort)) {
						if ((game->Squares[castleBlackOffset + 7] & 7) == ROOK &&
							game->Squares[castleBlackOffset + 5] == NOPIECE &&
							game->Squares[castleBlackOffset + 6] == NOPIECE)
						{
							if (!SquareAttacked(5 + castleBlackOffset, game->Side ^ 24, game) && !SquareAttacked(4 + castleBlackOffset, game->Side ^ 24, game))
								CreateMove(i, 6 + castleBlackOffset, CastleShort, game);
						}
					}
					if ((game->Side & WHITE && game->State & WhiteCanCastleLong) || (game->Side & BLACK && game->State & BlackCanCastleLong)) {
						if ((game->Squares[castleBlackOffset] & 7) == ROOK &&
							game->Squares[castleBlackOffset + 1] == NOPIECE &&
							game->Squares[castleBlackOffset + 2] == NOPIECE &&
							game->Squares[castleBlackOffset + 3] == NOPIECE)
						{
							if (!SquareAttacked(4 + castleBlackOffset, game->Side ^ 24, game) && !SquareAttacked(3 + castleBlackOffset, game->Side ^ 24, game))
								CreateMove(i, 2 + castleBlackOffset, CastleLong, game);
						}
					}
				}
				break;
			}
			default:
			{
				int pat = pt - 1;
				int raysCount = PieceTypeSquareRaysPatterns[pat][i][0][0];
				for (int r = 1; r <= raysCount; r++)
				{
					int rayLength = PieceTypeSquareRaysPatterns[pat][i][r][0];
					for (int rr = 1; rr <= rayLength; rr++)
					{
						int toSquare = PieceTypeSquareRaysPatterns[pat][i][r][rr];
						PieceType toPiece = game->Squares[toSquare];
						MoveInfo moveInfo = pt == ROOK ? RookMove : PlainMove;

						if (toPiece != NOPIECE) {
							if (!(toPiece & game->Side)) {
								CreateMove(i, toSquare, moveInfo, game);
							}
							break;
						}
						else {
							CreateMove(i, toSquare, moveInfo, game);
						}
					}
				}
				break;
			}
			}
		}
	}
	SortMoves(game->MovesBuffer, game->MovesBufferLength, game);
}

void CreateCaptureMoves(Game * game) {
	game->MovesBufferLength = 0;
	for (int i = 0; i < 64; i++)
	{
		PieceType pieceType = game->Squares[i];
		if (pieceType & game->Side) {
			PieceType pt = pieceType & 7;
			char otherSide = game->Side ^ 24;
			switch (pt)
			{
			case PAWN:
			{
				int captPat = PawnCapturePattern[game->Side >> 4];
				int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
				for (int pc = 1; pc <= pawnCapPatLength; pc++)
				{
					int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
					//Must be a piece of opposite color.
					if (game->Squares[toSquare] & (game->Side ^ 24))
					{
						if (toSquare < 8 || toSquare > 55) {
							CreateMove(i, toSquare, PromotionQueen, game);
							CreateMove(i, toSquare, PromotionRook, game);
							CreateMove(i, toSquare, PromotionBishop, game);
							CreateMove(i, toSquare, PromotionKnight, game);
						}
						else {
							CreateMove(i, toSquare, PlainMove, game);
						}
					}
					else {
						int enpFile = (game->State & 15) - 1;
						if (enpFile > -1) {
							int toFile = toSquare & 7;
							int toRank = toSquare >> 3;
							if (toFile == enpFile && toRank == EnpassantRankPattern[game->Side >> 4])
								CreateMove(i, toSquare, EnPassantCapture, game);
						}
					}
				}
				break;
			}
			case KNIGHT:
			{
				int length = PieceTypeSquarePatterns[0][i][0];
				for (int p = 1; p <= length; p++)
				{
					int toSquare = PieceTypeSquarePatterns[0][i][p];
					if (game->Squares[toSquare] & otherSide) {
						CreateMove(i, toSquare, 0, game);
					}
				}
				break;
			}
			case KING:
			{
				int length = PieceTypeSquarePatterns[1][i][0];
				for (int p = 1; p <= length; p++)
				{
					int toSquare = PieceTypeSquarePatterns[1][i][p];
					if (game->Squares[toSquare] & otherSide) {
						CreateMove(i, toSquare, KingMove, game);
					}
				}
				break;
			}
			default:
			{
				int pat = pt - 1;
				int raysCount = PieceTypeSquareRaysPatterns[pat][i][0][0];
				for (int r = 1; r <= raysCount; r++)
				{
					int rayLength = PieceTypeSquareRaysPatterns[pat][i][r][0];
					for (int rr = 1; rr <= rayLength; rr++)
					{
						int toSquare = PieceTypeSquareRaysPatterns[pat][i][r][rr];
						PieceType toPiece = game->Squares[toSquare];
						MoveInfo moveInfo = pt == ROOK ? RookMove : PlainMove;
						if (toPiece & otherSide) {
							CreateMove(i, toSquare, moveInfo, game);
							break;
						}
					}
				}
				break;
			}
			}
		}
	}
}

int Perft(depth) {
	if (depth == 0)
	{
		return 1;
	}
	int nodeCount = 0;
	CreateMoves(&mainGame);
	if (mainGame.MovesBufferLength == 0)
		return nodeCount;
	int count = mainGame.MovesBufferLength;
	Move * localMoves = malloc(count * MOVESIZE);
	memcpy(localMoves, mainGame.MovesBuffer, count * MOVESIZE);
	for (int i = 0; i < count; i++)
	{
		Move move = localMoves[i];
		PieceType capture = mainGame.Squares[To(move)];
		if (depth == 1) {
			MoveInfo info = Info(move);
			if (capture != NOPIECE)
				mainGame.PerftResult.Captures++;
			if (info == EnPassantCapture)
				mainGame.PerftResult.Captures++;
			if (info == CastleLong || info == CastleShort)
				mainGame.PerftResult.Castles++;
			if (info >= PromotionQueen && info <= PromotionKnight)
				mainGame.PerftResult.Promotions++;
			if (info == EnPassantCapture)
				mainGame.PerftResult.Enpassants++;
		}

		GameState prevGameState = mainGame.State;
		int prevPositionScore = mainGame.PositionScore;
		MakeMove(move, &mainGame);
		nodeCount += Perft(depth - 1);
		UnMakeMove(move, capture, prevGameState, prevPositionScore, &mainGame);
	}
	free(localMoves);
	return nodeCount;
}

PieceType parsePieceType(char c) {
	switch (c)
	{
	case 'p': return PAWN | BLACK;
	case 'r': return ROOK | BLACK;
	case 'b': return BISHOP | BLACK;
	case 'n': return KNIGHT | BLACK;
	case 'q': return QUEEN | BLACK;
	case 'k': return KING | BLACK;
	case 'P': return PAWN | WHITE;
	case 'R': return ROOK | WHITE;
	case 'B': return BISHOP | WHITE;
	case 'N': return KNIGHT | WHITE;
	case 'Q': return QUEEN | WHITE;
	case 'K': return KING | WHITE;

	default:
		return NOPIECE;
	}
}

PieceType parseSide(char c) {
	switch (c)
	{
	case 'w': return WHITE;
	case 'b': return BLACK;
	default:
		return NOPIECE;
	}
}

Move parseMove(char * sMove, MoveInfo info) {
	int fromFile = sMove[0] - 'a';
	int fromRank = sMove[1] - '1';
	int toFile = sMove[3] - 'a';
	int toRank = sMove[4] - '1';
	Move move = 0;
	move |= ((fromRank * 8 + fromFile) << 16);
	move |= ((toRank * 8 + toFile) << 22);
	move |= (info << 28);
	return move;
}

void InitScores() {
	mainGame.Material[0] = 0;
	mainGame.Material[1] = 0;
	mainGame.PositionScore = 0;

	for (int i = 0; i < 64; i++)
	{
		PieceType pt = mainGame.Squares[i] & 7;
		int colorSide = (mainGame.Squares[i] & (WHITE | BLACK)) >> 4;
		mainGame.Material[colorSide] += MaterialMatrix[colorSide][pt];
		mainGame.PositionScore += PositionValueMatrix[pt][colorSide][i];
	}

	//aproximation that endgame starts att 1900.
	int endGame = mainGame.Material[1] - mainGame.Material[0] < 1900 ? 1 : 0;

	mainGame.PositionScore += KingPositionValueMatrix[endGame][0][mainGame.KingSquares[0]];
	mainGame.PositionScore += KingPositionValueMatrix[endGame][1][mainGame.KingSquares[1]];
}

void ReadFen(char * fen) {
	//rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
	for (size_t i = 0; i < 64; i++)
		mainGame.Squares[i] = NOPIECE;
	int index = 0;
	int file = 0;
	int rank = 7;
	while (fen[index] != ' ' && fen[index])
	{
		char c = fen[index];
		index++;
		if (isdigit(c)) {
			int dig = parseChar(c);
			file += dig;
		}
		else if (c == '/') {
			rank--;
			file = 0;
		}
		else {
			mainGame.Squares[rank * 8 + file] = parsePieceType(c);
			file++;
		}
	}

	index++;
	mainGame.Side = parseSide(fen[index]);
	index++;
	index++;
	mainGame.State = 0;
	while (fen[index] != ' ')
	{
		switch (fen[index])
		{
		case 'K': mainGame.State |= WhiteCanCastleShort;
			break;
		case 'Q': mainGame.State |= WhiteCanCastleLong;
			break;
		case 'k': mainGame.State |= BlackCanCastleShort;
			break;
		case 'q': mainGame.State |= BlackCanCastleLong;
			break;
		default:
			break;
		}
		index++;
	}
	index++;
	char enpFile = fen[index] - 'a';
	if (enpFile >= 0 && enpFile <= 8)
		mainGame.State |= (enpFile + 1);
	//todo: counters	

	for (int i = 0; i < 64; i++)
	{
		if ((mainGame.Squares[i] & 7) == KING) {
			int color = (mainGame.Squares[i] >> 4);
			mainGame.KingSquares[color] = i;
		}
	}

	InitScores();
}

void WriteFen(char * fenBuffer) {
	int index = 0;
	for (int rank = 8 - 1; rank >= 0; rank--)
	{
		for (int file = 0; file < 8; file++)
		{
			int emptyCount = 0;
			while (mainGame.Squares[rank * 8 + file] == NOPIECE && file < 8)
			{
				emptyCount++;
				file++;
			}

			if (emptyCount > 0) {
				fenBuffer[index++] = '0' + emptyCount;
				file--;
			}
			else {
				fenBuffer[index++] = PieceChar(mainGame.Squares[rank * 8 + file]);
			}
		}
		if (rank > 0)
			fenBuffer[index++] = '/';
	}
	fenBuffer[index++] = ' ';
	fenBuffer[index++] = mainGame.Side == WHITE ? 'w' : 'b';
	fenBuffer[index++] = ' ';
	if (mainGame.State & WhiteCanCastleShort) fenBuffer[index++] = 'K';
	if (mainGame.State & WhiteCanCastleLong) fenBuffer[index++] = 'Q';
	if (mainGame.State & BlackCanCastleShort) fenBuffer[index++] = 'k';
	if (mainGame.State & BlackCanCastleLong) fenBuffer[index++] = 'q';
	fenBuffer[index++] = ' ';
	char enPassantFile = (mainGame.State & 15) + '0';
	if (enPassantFile == '0')
		enPassantFile = '-';
	fenBuffer[index++] = enPassantFile;
	fenBuffer[index] = '\0';
}

int ValidMoves(Move * moves) {
	CreateMoves(&mainGame);
	if (mainGame.MovesBufferLength == 0)
		return 0;

	memcpy(moves, mainGame.MovesBuffer, mainGame.MovesBufferLength * MOVESIZE);
	return mainGame.MovesBufferLength;
}

PlayerMove MakePlayerMove(char * sMove) {
	Move move = parseMove(sMove, 0);
	Move moves[100];
	int length = ValidMoves(moves);
	PlayerMove playerMove;
	int fromToMask = 0xFFF0000;
	for (int i = 0; i < length; i++)
	{
		if ((moves[i] & fromToMask) == (move & fromToMask)) {
			playerMove.Move = moves[i];
			playerMove.Capture = mainGame.Squares[To(move)];
			playerMove.PreviousGameState = mainGame.State;
			playerMove.Invalid = false;
			playerMove.PreviousPositionScore = mainGame.PositionScore;
			MakeMove(moves[i], &mainGame);
			return playerMove;
		}
	}
	playerMove.Invalid = true;
	return playerMove;
}

void UnMakePlayerMove(PlayerMove playerMove) {
	UnMakeMove(playerMove.Move, playerMove.Capture, playerMove.PreviousGameState, playerMove.PreviousPositionScore, &mainGame);
}

short TotalMaterial(Game * game) {
	return game->Material[0] + game->Material[1];
}

void SwitchSignOfWhitePositionValue() {
	for (int i = 1; i < 7; i++)
	{
		for (int rnd_seed = 0; rnd_seed < 64; rnd_seed++)
		{
			PositionValueMatrix[i][0][rnd_seed] = -PositionValueMatrix[i][0][rnd_seed];
		}
	}

	for (int i = 0; i < 64; i++)
	{
		KingPositionValueMatrix[0][0][i] = -KingPositionValueMatrix[0][0][i];
		KingPositionValueMatrix[1][0][i] = -KingPositionValueMatrix[1][0][i];
	}
}

short GetScore(Game * game) {
	return game->Material[0] + game->Material[1] + game->PositionScore;
}

int AlphaBetaQuite(int alpha, int beta, int depth, Game * game) {
	if (!depth) {
		SearchedLeafs++;
		return GetScore(game);
	}

	int bestVal = 0;
	CreateCaptureMoves(game);
	int moveCount = game->MovesBufferLength;
	Move * localMoves = malloc(moveCount * MOVESIZE);
	memcpy(localMoves, game->MovesBuffer, moveCount * MOVESIZE);
	if (moveCount == 0) {
		free(localMoves);
		return GetScore(game);
	}
	else if (game->Side == BLACK) { //maximizing
		bestVal = alpha;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			PieceType capture = game->Squares[To(childMove)];
			GameState state = game->State;
			int prevPosScore = game->PositionScore;

			MakeMove(childMove, game);
			int childValue = AlphaBetaQuite(bestVal, beta, depth - 1, game);
			UnMakeMove(childMove, capture, state, prevPosScore, game);
			bestVal = max(bestVal, childValue);
			if (bestVal >= beta)
				break;
		}
	}
	else { //minimizing
		bestVal = beta;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			PieceType capture = game->Squares[To(childMove)];
			GameState state = game->State;
			int prevPosScore = game->PositionScore;
			MakeMove(childMove, game);
			int childValue = AlphaBetaQuite(alpha, bestVal, depth - 1, game);
			UnMakeMove(childMove, capture, state, prevPosScore, game);
			bestVal = min(bestVal, childValue);
			if (bestVal <= alpha)
				break;
		}
	}
	free(localMoves);
	return bestVal;
}

short AlphaBeta(short alpha, short beta, int depth, PieceType capture, Game * game) {
	if (!depth) {
		SearchedLeafs++;
		if (capture) {
			return AlphaBetaQuite(alpha, beta, 1, game);
		}
		return GetScore(game);
	}
	short bestVal = 0;
	CreateMoves(game);
	int moveCount = game->MovesBufferLength;
	Move * localMoves = malloc(moveCount * MOVESIZE);
	memcpy(localMoves, game->MovesBuffer, moveCount * MOVESIZE);
	if (moveCount == 0) {
		if (SquareAttacked(game->KingSquares[game->Side >> 4], game->Side ^ 24, game)) {
			//mate
			bestVal = game->Side == WHITE ? 8000 : -8000;
		}
		else {
			//stale mate
			bestVal = 0;
		}
	}
	else if (game->Side == BLACK) { //maximizing
		bestVal = alpha;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			PieceType capture = game->Squares[To(childMove)];
			GameState state = game->State;
			int prevPosScore = game->PositionScore;

			MakeMove(childMove, game);
			int childValue = AlphaBeta(bestVal, beta, depth - 1, capture, game);
			UnMakeMove(childMove, capture, state, prevPosScore, game);
			bestVal = max(bestVal, childValue);
			if (bestVal >= beta)
				break;
		}
	}
	else { //minimizing
		bestVal = beta;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			PieceType capture = game->Squares[To(childMove)];
			GameState state = game->State;
			int prevPosScore = game->PositionScore;
			MakeMove(childMove, game);
			int childValue = AlphaBeta(alpha, bestVal, depth - 1, capture, game);
			UnMakeMove(childMove, capture, state, prevPosScore, game);
			bestVal = min(bestVal, childValue);
			if (bestVal <= alpha)
				break;
		}
	}
	free(localMoves);
	return bestVal;
}

Game * CopyMainGame(int threadNo) {
	threadGames[threadNo] = mainGame;
	threadGames[threadNo].KingSquares[0] = mainGame.KingSquares[0];
	threadGames[threadNo].KingSquares[1] = mainGame.KingSquares[1];
	threadGames[threadNo].Material[0] = mainGame.Material[0];
	threadGames[threadNo].Material[1] = mainGame.Material[1];
	memcpy(mainGame.MovesBuffer, threadGames[threadNo].MovesBuffer, mainGame.MovesBufferLength * MOVESIZE);
	memcpy(mainGame.Squares, threadGames[threadNo].Squares, 64 * sizeof(PieceType));
	return &threadGames[threadNo];
}

DWORD WINAPI SearchThread(ThreadParams * prm) {
	//ThreadParams params = *prm;
	do
	{
		Game * game = CopyMainGame(prm->threadID);
		Move move = prm->moves[prm->moveIndex];
		PieceType capt = game->Squares[To(move)];
		GameState gameState = game->State;
		int positionScore = game->PositionScore;

		MakeMove(move, game);
		short scr = (short)(move);
		int alpha = scr - prm->window;
		int beta = scr + prm->window;
		short score = AlphaBeta(alpha, beta, prm->depth, capt, game);
		if (score < alpha || score > beta) {
			prm->window = 8000;
			UnMakeMove(move, capt, gameState, positionScore, game);
			continue;
		}
		if (prm->depth > 5)
			prm->window = 15;

		move = (move >> 16) << 16;
		move |= (score & 0xFFFF);
		*(&prm->moves[prm->moveIndex]) = move;

		UnMakeMove(move, capt, gameState, positionScore, game);
		if ((game->Side == WHITE && score < -7000) || (game->Side == BLACK && score > 7000))
		{
			return 0;
		}
		prm->moveIndex += SEARCH_THREADS;
	} while (prm->moveIndex < prm->moveCount);

	return 0;
}

void SetMovesScoreAtDepth(int depth, Move * localMoves, int moveCount) {
	int window = 8000;
	if (depth > 5)
		window = 15;
	else
		window = 8000;

	int moveIndex = 0;
	//starta en tråd per drag
	//starta inte fler trådar än 8 (kärnor)
	//när en tråd är klar, starta nästa
	//när alla trådar är klara returnera
	HANDLE threadHandles[SEARCH_THREADS];
	ThreadParams params[SEARCH_THREADS];

	for (int i = 0; i < SEARCH_THREADS; i++)
	{
		if (i > moveCount) //in case more threads than moves
			break;

		params[i].threadID = i;
		params[i].depth = depth;
		params[i].moves = localMoves;
		params[i].moveIndex = i;
		params[i].moveCount = moveCount;
		params[i].window = window;

		threadHandles[i] = CreateThread(NULL, 0, SearchThread, &params[i], 0, NULL);
		//todo: error handling
	}
	WaitForMultipleObjects(SEARCH_THREADS, threadHandles, TRUE, INFINITE);
	
}

Move BestMove(Move * moves, int moveCount) {
	int bestValue = mainGame.Side == WHITE ? 9000 : -9000;
	Move bestMove;
	for (int i = 0; i < moveCount; i++)
	{
		Move move = moves[i];
		short score = (short)move;
		if (mainGame.Side == WHITE) {
			if (score < bestValue)
			{
				bestValue = score;
				bestMove = move;
			}
		}
		else {
			if (score > bestValue)
			{
				bestValue = score;
				bestMove = move;
			}
		}
	}
	return bestMove;
}

Move BestMoveAtDepthDeepening(int maxDepth) {
	CreateMoves(&mainGame);
	int moveCount = mainGame.MovesBufferLength;
	Move * localMoves = malloc(moveCount * MOVESIZE);
	memcpy(localMoves, mainGame.MovesBuffer, moveCount * MOVESIZE);

	int depth = 1;
	do
	{
		SetMovesScoreAtDepth(depth, localMoves, moveCount);
		SortMoves(localMoves, moveCount, &mainGame);

		depth++;
	} while (depth <= maxDepth); //todo: continue until time ends
	return localMoves[0];
}

void computerMove() {
	Move move = BestMoveAtDepthDeepening(6);
	MakeMove(move, &mainGame);
}

int main() {
	SwitchSignOfWhitePositionValue();
	InitGame();
	char scan = 0;
	while (scan != 'q')
	{
		PrintGame();
		printf("m: make move\n");
		printf("c: computer move\n");
		printf("t: run tests\n");
		printf("q: quit\n");
		scanf_s(" %c", &scan, 1);
		system("@cls||clear");
		switch (scan)
		{
		case 'm':
			break;
		case 'c':
			computerMove();
			break;
		case 't':
			runTests();
			break;
		case 'p':
			break;
		case 'q':
			break;
		default:
			break;
		}
	}

	return 0;
}