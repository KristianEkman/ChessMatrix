#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#include "basic_structs.h"
#include "patterns.h"
#include "main.h"
#include "utils.h"
#include "tests.h"
#include "evaluation.h"
#include "sort.h"

const int MOVESIZE = sizeof(Move);
Game game;
int _behind[] = { -8, 8 };
int SearchedLeafs = 0;

void InitPiece(int file, int rank, enum PieceType type, enum Color color) {
	game.Squares[rank * 8 + file] = type | color;
}

void InitGame() {
	for (int i = 0; i < 64; i++)
		game.Squares[i] = NOPIECE;

	InitPiece(0, 0, ROOK, WHITE);
	InitPiece(1, 0, KNIGHT, WHITE);
	InitPiece(2, 0, BISHOP, WHITE);
	InitPiece(3, 0, QUEEN, WHITE);
	InitPiece(4, 0, KING, WHITE);
	game.KingSquares[0] = 4;
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
	game.KingSquares[1] = 60;
	InitPiece(5, 7, BISHOP, BLACK);
	InitPiece(6, 7, KNIGHT, BLACK);
	InitPiece(7, 7, ROOK, BLACK);

	for (int i = 0; i < 8; i++)
		InitPiece(i, 6, PAWN, BLACK);
	game.Side = WHITE;

	game.State = WhiteCanCastleLong | WhiteCanCastleShort | BlackCanCastleLong | BlackCanCastleShort;
	game.Material[0] = 0;
	game.Material[1] = 0;

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
			PieceType piece = game.Squares[r * 8 + f];
			char c = PieceChar(piece);
			printf("| %c ", c);
		}
		printf("|\n  ---------------------------------\n");
	}
	printf("    a   b   c   d   e   f   g   h  \n");
}

void KingPositionScore(Move move) {
	//aproximation that endgame starts att 1900.
	int endGame = game.Material[1] - game.Material[0] < 1900 ? 1 : 0;
	game.PositionScore += KingPositionValueMatrix[endGame][game.Side >> 4][move.To];
	game.PositionScore -= KingPositionValueMatrix[endGame][game.Side >> 4][move.From];
}

void MakeMove(Move move) {
	char f = move.From;
	char t = move.To;

	PieceType captType = game.Squares[t];
	int captColor = captType >> 4;
	int side01 = game.Side >> 4;

	//Capturing
	game.Material[captColor] -= MaterialMatrix[captColor][captType & 7];

	//removing piece from square removes its position score
	game.PositionScore -= PositionValueMatrix[captType & 7][captColor][t];

	PieceType pieceType = game.Squares[f];

	char pt = pieceType & 7;
	game.PositionScore -= PositionValueMatrix[pt][side01][f];
	game.PositionScore += PositionValueMatrix[pt][side01][t];

	game.Squares[t] = game.Squares[f];
	game.Squares[f] = NOPIECE;
	
	//resetting en passant every move
	game.State &= ~15;

	switch (move.MoveInfo)
	{
	case PromotionQueen:
		game.Squares[t] = QUEEN | game.Side;
		game.Material[side01] += MaterialMatrix[side01][QUEEN + 6];
		game.PositionScore += PositionValueMatrix[QUEEN][side01][t];
		break;
	case PromotionRook:
		game.Squares[t] = ROOK | game.Side;
		game.Material[side01] += MaterialMatrix[side01][ROOK + 6];
		game.PositionScore += PositionValueMatrix[ROOK][side01][t];
		break;
	case PromotionBishop:
		game.Squares[t] = BISHOP | game.Side;
		game.Material[side01] += MaterialMatrix[side01][BISHOP + 6];
		game.PositionScore += PositionValueMatrix[BISHOP][side01][t];
		break;
	case PromotionKnight:
		game.Squares[move.To] = KNIGHT | game.Side;
		game.Material[side01] += MaterialMatrix[side01][KNIGHT + 6];
		game.PositionScore += PositionValueMatrix[KNIGHT][side01][t];
		break;
	case KingMove:
		game.KingSquares[side01] = t;
		KingPositionScore(move);
		break;
	case RookMove:
		switch (move.From)
		{
		case 0:
			game.State &= ~WhiteCanCastleLong;
			break;
		case 7:
			game.State &= ~WhiteCanCastleShort;
			break;
		case 56:
			game.State &= ~BlackCanCastleLong;
			break;
		case 63:
			game.State &= ~BlackCanCastleShort;
			break;
		default:
			break;
		}
		break;
	case CastleShort:
	{
		game.KingSquares[side01] = t;
		char rookFr = 7 + CastlesOffset[side01];
		char rookTo = 5 + CastlesOffset[side01];
		PieceType rook = ROOK | game.Side;
		game.Squares[rookFr] = NOPIECE;
		game.Squares[rookTo] = rook;

		game.PositionScore -= PositionValueMatrix[ROOK][side01][rookFr];
		game.PositionScore += PositionValueMatrix[ROOK][side01][rookTo];
		KingPositionScore(move);
	}
	break;
	case CastleLong:
	{
		game.KingSquares[side01] = t;
		char rookFr = CastlesOffset[side01];
		char rookTo = 3 + CastlesOffset[side01];
		PieceType rook = ROOK | game.Side;
		game.Squares[rookFr] = NOPIECE;
		game.Squares[rookTo] = ROOK | game.Side;

		game.PositionScore -= PositionValueMatrix[ROOK][side01][rookFr];
		game.PositionScore += PositionValueMatrix[ROOK][side01][rookTo];
		KingPositionScore(move);
	}
	break;
	case EnPassant:
		game.State |= ((move.From & 7) + 1); //Sets the file. a to h file is 1 to 8.
		break;
	case EnPassantCapture:
	{
		char behind = t + _behind[side01];
		game.Squares[behind] = NOPIECE;
		game.Material[side01] += MaterialMatrix[side01][PAWN];
		game.PositionScore -= PositionValueMatrix[PAWN][captColor][behind];
	}
	break;
	default:
		break;
	}
	game.Side ^= 24;
}

void UnMakeMove(Move move, PieceType capture, GameState prevGameState, int prevPositionScore) {

	game.Material[capture >> 4] += MaterialMatrix[capture >> 4][capture & 7];

	game.Squares[move.From] = game.Squares[move.To];
	game.Squares[move.To] = capture;
	int otherSide = game.Side ^ 24;
	int otherSide01 = otherSide >> 4;
	switch (move.MoveInfo)
	{
	case PromotionQueen:
		game.Material[otherSide01] -= MaterialMatrix[otherSide01][QUEEN + 6];
		game.Squares[move.From] = PAWN | otherSide;
		break;
	case PromotionRook:
		game.Material[otherSide01] -= MaterialMatrix[otherSide01][ROOK + 6];
		game.Squares[move.From] = PAWN | otherSide;
		break;
	case PromotionBishop:
		game.Material[otherSide01] -= MaterialMatrix[otherSide01][BISHOP + 6];
		game.Squares[move.From] = PAWN | otherSide;
		break;
	case PromotionKnight:
		game.Material[otherSide01] -= MaterialMatrix[otherSide01][KNIGHT + 6];
		game.Squares[move.From] = PAWN | otherSide;
		break;
	case KingMove:
		game.KingSquares[otherSide01] = move.From;
		break;
	case CastleShort:
		game.KingSquares[otherSide01] = move.From;
		game.Squares[5 + CastlesOffset[otherSide01]] = NOPIECE;
		game.Squares[7 + CastlesOffset[otherSide01]] = ROOK | otherSide;
		break;
	case CastleLong:
		game.KingSquares[otherSide01] = move.From;
		game.Squares[3 + CastlesOffset[otherSide01]] = NOPIECE;
		game.Squares[0 + CastlesOffset[otherSide01]] = ROOK | otherSide;
		break;
	case EnPassantCapture:
		game.Squares[move.To + _behind[otherSide01]] = PAWN | game.Side;
		game.Material[otherSide01] -= MaterialMatrix[otherSide01][PAWN];
		break;
	default:
		break;
	}
	game.State = prevGameState;
	game.PositionScore = prevPositionScore;
	game.Side ^= 24;
}


bool SquareAttacked(int square, char attackedBy) {
	for (int i = 0; i < 64; i++)
	{
		PieceType pieceType = game.Squares[i];
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
				for (int rnd_seed = 1; rnd_seed <= rayLength; rnd_seed++)
				{
					int toSquare = PieceTypeSquareRaysPatterns[pat][i][r][rnd_seed];
					if (toSquare == square)
						return true;
					if (game.Squares[toSquare] > NOPIECE)
						break;
				}
			}
			break;
		}
		}

	}
	return false;
}

void SortMoves(Move * moves, int moveCount) {
	if (game.Side == WHITE)
		QuickSort(moves, 0, moveCount - 1);
	else
		QuickSortDescending(moves, 0, moveCount - 1);
}

void CreateMove(int fromSquare, int toSquare, MoveInfo moveInfo) {
	PieceType capture = game.Squares[toSquare];
	GameState prevGameState = game.State;
	Move move;
	move.From = fromSquare;
	move.To = toSquare;
	move.MoveInfo = moveInfo;
	int prevPosScore = game.PositionScore;
	MakeMove(move);
	move.ScoreAtDepth = GetScore();
	int  kingSquare = game.KingSquares[(game.Side ^ 24) >> 4];
	bool legal = !SquareAttacked(kingSquare, game.Side);
	if (legal)
		game.MovesBuffer[game.MovesBufferLength++] = move;

	UnMakeMove(move, capture, prevGameState, prevPosScore);
}

void CreateMoves() {
	game.MovesBufferLength = 0;
	for (int i = 0; i < 64; i++)
	{
		PieceType pieceType = game.Squares[i];
		if (pieceType != NOPIECE && (pieceType & game.Side)) {
			PieceType pt = pieceType & 7;
			switch (pt)
			{
			case PAWN:
			{
				int pat = PawnPattern[game.Side >> 4];
				int pawnPatLength = PieceTypeSquarePatterns[pat][i][0];
				for (int pp = 1; pp <= pawnPatLength; pp++)
				{
					int toSquare = PieceTypeSquarePatterns[pat][i][pp];
					if (game.Squares[toSquare] != NOPIECE)
						break;
					if (toSquare < 8 || toSquare > 55) {
						CreateMove(i, toSquare, PromotionQueen);
						CreateMove(i, toSquare, PromotionRook);
						CreateMove(i, toSquare, PromotionBishop);
						CreateMove(i, toSquare, PromotionKnight);
					}
					else if (pp == 2) {
						CreateMove(i, toSquare, EnPassant);
					}
					else {
						CreateMove(i, toSquare, PlainMove);
					}
				}

				int captPat = PawnCapturePattern[game.Side >> 4];
				int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
				for (int pc = 1; pc <= pawnCapPatLength; pc++)
				{
					int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
					//Must be a piece of opposite color.
					if (game.Squares[toSquare] & (game.Side ^ 24))
					{
						if (toSquare < 8 || toSquare > 55) {
							CreateMove(i, toSquare, PromotionQueen);
							CreateMove(i, toSquare, PromotionRook);
							CreateMove(i, toSquare, PromotionBishop);
							CreateMove(i, toSquare, PromotionKnight);
						}
						else {
							CreateMove(i, toSquare, PlainMove);
						}
					}
					else {
						int enpFile = (game.State & 15) - 1;
						if (enpFile > -1) {
							int toFile = toSquare & 7;
							int toRank = toSquare >> 3;
							if (toFile == enpFile && toRank == EnpassantRankPattern[game.Side >> 4])
								CreateMove(i, toSquare, EnPassantCapture);
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
					if (!(game.Squares[toSquare] & game.Side)) {
						CreateMove(i, toSquare, 0);
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
					if (!(game.Squares[toSquare] & game.Side)) {
						CreateMove(i, toSquare, KingMove);
					}
				}

				int castleBlackOffset = CastlesOffset[game.Side >> 4];
				if (i == castleBlackOffset + 4) { //King on origin pos
					if ((game.Side & WHITE && game.State & WhiteCanCastleShort) || (game.Side & BLACK && game.State & BlackCanCastleShort)) {
						if ((game.Squares[castleBlackOffset + 7] & 7) == ROOK &&
							game.Squares[castleBlackOffset + 5] == NOPIECE &&
							game.Squares[castleBlackOffset + 6] == NOPIECE)
						{
							if (!SquareAttacked(5 + castleBlackOffset, game.Side ^ 24) && !SquareAttacked(4 + castleBlackOffset, game.Side ^ 24))
								CreateMove(i, 6 + castleBlackOffset, CastleShort);
						}
					}
					if ((game.Side & WHITE && game.State & WhiteCanCastleLong) || (game.Side & BLACK && game.State & BlackCanCastleLong)) {
						if ((game.Squares[castleBlackOffset] & 7) == ROOK &&
							game.Squares[castleBlackOffset + 1] == NOPIECE &&
							game.Squares[castleBlackOffset + 2] == NOPIECE &&
							game.Squares[castleBlackOffset + 3] == NOPIECE)
						{
							if (!SquareAttacked(4 + castleBlackOffset, game.Side ^ 24) && !SquareAttacked(3 + castleBlackOffset, game.Side ^ 24))
								CreateMove(i, 2 + castleBlackOffset, CastleLong);
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
					for (int rnd_seed = 1; rnd_seed <= rayLength; rnd_seed++)
					{
						int toSquare = PieceTypeSquareRaysPatterns[pat][i][r][rnd_seed];
						PieceType toPiece = game.Squares[toSquare];
						MoveInfo moveInfo = pt == ROOK ? RookMove : PlainMove;

						if (toPiece != NOPIECE) {
							if (!(toPiece & game.Side)) {
								CreateMove(i, toSquare, moveInfo);
							}
							break;
						}
						else {
							CreateMove(i, toSquare, moveInfo);
						}
					}
				}
				break;
			}
			}
		}
	}
	SortMoves(game.MovesBuffer, game.MovesBufferLength);
}

int Perft(depth) {
	if (depth == 0)
	{
		return 1;
	}
	int nodeCount = 0;
	CreateMoves();
	if (game.MovesBufferLength == 0)
		return nodeCount;
	int count = game.MovesBufferLength;
	Move * localMoves = malloc(count * MOVESIZE);
	memcpy(localMoves, game.MovesBuffer, count * MOVESIZE);
	for (int i = 0; i < count; i++)
	{
		Move move = localMoves[i];
		PieceType capture = game.Squares[move.To];
		if (depth == 1) {
			if (capture != NOPIECE)
				game.PerftResult.Captures++;
			if (move.MoveInfo == EnPassantCapture)
				game.PerftResult.Captures++;
			if (move.MoveInfo == CastleLong || move.MoveInfo == CastleShort)
				game.PerftResult.Castles++;
			if (move.MoveInfo >= PromotionQueen && move.MoveInfo <= PromotionKnight)
				game.PerftResult.Promotions++;
			if (move.MoveInfo == EnPassantCapture)
				game.PerftResult.Enpassants++;
		}

		GameState prevGameState = game.State;
		int prevPositionScore = game.PositionScore;
		MakeMove(move);
		nodeCount += Perft(depth - 1);
		UnMakeMove(move, capture, prevGameState, prevPositionScore);
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
	Move move;
	move.From = fromRank * 8 + fromFile;
	move.To = toRank * 8 + toFile;
	move.MoveInfo = info;
	return move;
}

void InitScores() {
	game.Material[0] = 0;
	game.Material[1] = 0;
	game.PositionScore = 0;

	for (int i = 0; i < 64; i++)
	{
		PieceType pt = game.Squares[i] & 7;
		int colorSide = (game.Squares[i] & (WHITE | BLACK)) >> 4;
		game.Material[colorSide] += MaterialMatrix[colorSide][pt];
		game.PositionScore += PositionValueMatrix[pt][colorSide][i];
	}

	//aproximation that endgame starts att 1900.
	int endGame = game.Material[1] - game.Material[0] < 1900 ? 1 : 0;

	game.PositionScore += KingPositionValueMatrix[endGame][0][game.KingSquares[0]];
	game.PositionScore += KingPositionValueMatrix[endGame][1][game.KingSquares[1]];
}

void ReadFen(char * fen) {
	//rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
	for (size_t i = 0; i < 64; i++)
		game.Squares[i] = NOPIECE;
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
			game.Squares[rank * 8 + file] = parsePieceType(c);
			file++;
		}
	}

	index++;
	game.Side = parseSide(fen[index]);
	index++;
	index++;
	game.State = 0;
	while (fen[index] != ' ')
	{
		switch (fen[index])
		{
		case 'K': game.State |= WhiteCanCastleShort;
			break;
		case 'Q': game.State |= WhiteCanCastleLong;
			break;
		case 'k': game.State |= BlackCanCastleShort;
			break;
		case 'q': game.State |= BlackCanCastleLong;
			break;
		default:
			break;
		}
		index++;
	}
	index++;
	char enpFile = fen[index] - 'a';
	if (enpFile >= 0 && enpFile <= 8)
		game.State |= (enpFile + 1);
	//todo: counters	

	for (int i = 0; i < 64; i++)
	{
		if ((game.Squares[i] & 7) == KING) {
			int color = (game.Squares[i] >> 4);
			game.KingSquares[color] = i;
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
			while (game.Squares[rank * 8 + file] == NOPIECE && file < 8)
			{
				emptyCount++;
				file++;
			}

			if (emptyCount > 0) {
				fenBuffer[index++] = '0' + emptyCount;
				file--;
			}
			else {
				fenBuffer[index++] = PieceChar(game.Squares[rank * 8 + file]);
			}
		}
		if (rank > 0)
			fenBuffer[index++] = '/';
	}
	fenBuffer[index++] = ' ';
	fenBuffer[index++] = game.Side == WHITE ? 'w' : 'b';
	fenBuffer[index++] = ' ';
	if (game.State & WhiteCanCastleShort) fenBuffer[index++] = 'K';
	if (game.State & WhiteCanCastleLong) fenBuffer[index++] = 'Q';
	if (game.State & BlackCanCastleShort) fenBuffer[index++] = 'k';
	if (game.State & BlackCanCastleLong) fenBuffer[index++] = 'q';
	fenBuffer[index++] = ' ';
	char enPassantFile = (game.State & 15) + '0';
	if (enPassantFile == '0')
		enPassantFile = '-';
	fenBuffer[index++] = enPassantFile;
	fenBuffer[index] = '\0';
}

int ValidMoves(Move * moves) {
	CreateMoves();
	if (game.MovesBufferLength == 0)
		return 0;

	memcpy(moves, game.MovesBuffer, game.MovesBufferLength * MOVESIZE);
	return game.MovesBufferLength;
}

PlayerMove MakePlayerMove(char * sMove) {
	Move move = parseMove(sMove, 0);
	Move moves[100];
	int length = ValidMoves(moves);
	PlayerMove playerMove;
	for (int i = 0; i < length; i++)
	{
		if (moves[i].From == move.From && moves[i].To == move.To) {
			playerMove.Move = moves[i];
			playerMove.Capture = game.Squares[move.To];
			playerMove.PreviousGameState = game.State;
			playerMove.Invalid = false;
			playerMove.PreviousPositionScore = game.PositionScore;
			MakeMove(moves[i]);
			return playerMove;
		}
	}
	playerMove.Invalid = true;
	return playerMove;
}

void UnMakePlayerMove(PlayerMove playerMove) {
	UnMakeMove(playerMove.Move, playerMove.Capture, playerMove.PreviousGameState, playerMove.PreviousPositionScore);
}

short TotalMaterial() {
	return game.Material[0] + game.Material[1];
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

int GetScore() {
	return game.Material[0] + game.Material[1] + game.PositionScore;
}

int AlphaBeta(int alpha, int beta, int depth) {
	if (!depth) {
		SearchedLeafs++;
		//todo, is it worth probing database here? Dont think so.
		return GetScore();
	}
	int bestVal = 0;
	CreateMoves();
	int moveCount = game.MovesBufferLength;
	Move * localMoves = malloc(moveCount * MOVESIZE);
	memcpy(localMoves, game.MovesBuffer, moveCount * MOVESIZE);
	if (moveCount == 0) {
		if (SquareAttacked(game.KingSquares[game.Side >> 4], game.Side ^ 24)) {
			//mate
			bestVal = game.Side == WHITE ? 8000 : -8000;
		}
		else {
			//stale mate
			bestVal = 0;
		}
	}
	else if (game.Side == BLACK) { //maximizing
		bestVal = alpha;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			PieceType capture = game.Squares[childMove.To];
			GameState state = game.State;
			int prevPosScore = game.PositionScore;

			MakeMove(childMove);
			int childValue = AlphaBeta(bestVal, beta, depth - 1);
			UnMakeMove(childMove, capture, state, prevPosScore);
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
			PieceType capture = game.Squares[childMove.To];
			GameState state = game.State;
			int prevPosScore = game.PositionScore;
			MakeMove(childMove);
			int childValue = AlphaBeta(alpha, bestVal, depth - 1);
			UnMakeMove(childMove, capture, state, prevPosScore);
			bestVal = min(bestVal, childValue);
			if (bestVal <= alpha)
				break;
		}
	}
	return bestVal;
}

void SetMovesScoreAtDepth(int depth, Move * localMoves, int moveCount) {
	for (int i = 0; i < moveCount; i++)
	{
		Move move = localMoves[i];
		PieceType capt = game.Squares[move.To];
		GameState gameState = game.State;
		int positionScore = game.PositionScore;

		MakeMove(move);

		//todo: aspiration window here
		int score = AlphaBeta(-9000, 9000, depth);
		localMoves[i].ScoreAtDepth = score;
		UnMakeMove(move, capt, gameState, positionScore);
	}
}

Move BestMove(Move * moves, int moveCount) {
	int bestValue = game.Side == WHITE ? 9000 : -9000;
	Move bestMove;
	for (int i = 0; i < moveCount; i++)
	{
		Move move = moves[i];
		int score = move.ScoreAtDepth;
		if (game.Side == WHITE) {
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

Move BestMoveAtDepth(int depth) {
	CreateMoves();
	int moveCount = game.MovesBufferLength;
	Move * localMoves = malloc(moveCount * MOVESIZE);
	memcpy(localMoves, game.MovesBuffer, moveCount * MOVESIZE);

	SetMovesScoreAtDepth(depth, localMoves, moveCount);
	return BestMove(localMoves, moveCount);
}

Move BestMoveAtDepthDeepening(int maxDepth) {
	CreateMoves();
	int moveCount = game.MovesBufferLength;
	Move * localMoves = malloc(moveCount * MOVESIZE);
	memcpy(localMoves, game.MovesBuffer, moveCount * MOVESIZE);

	int depth = 1;
	do
	{
		SetMovesScoreAtDepth(depth, localMoves, moveCount);
		SortMoves(localMoves, moveCount);

		depth++;
	} while (depth <= maxDepth); //todo: continue until time ends
	return localMoves[0];
}

int main() {
	SwitchSignOfWhitePositionValue();
	InitGame();
	char rnd_seed = 0;
	while (rnd_seed != 'q')
	{
		PrintGame();
		printf("m: make move\n");
		printf("c: cpu move\n");
		printf("t: run tests\n");
		printf("q: quit\n");
		scanf_s(" %c", &rnd_seed, 1);
		system("@cls||clear");
		switch (rnd_seed)
		{
		case 'm':
			break;
		case 'c':
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

