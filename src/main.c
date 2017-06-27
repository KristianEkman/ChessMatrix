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

const int MOVESIZE = sizeof(Move);

//int _perftCount;
//PerftResult _perftResult;
Game game;
//char _side = WHITE;
//
////Keeps track of kings. Since kings square is checked against attack in legal evaluation so often this helps performance.
//int _kingSquares[2];
//
//PieceType _squares[64];
//
//int _movesBufferLength = 0;
//
////Buffer for move generation
////Assuming a position can never generate more than 100 moves.
//Move _movesBuffer[100];
//
//GameState _gameState;
//short GameMaterial = 0;

//one square behind another square, subtract by 8 for white(0) and add 8 for black(1).
int _behind[] = { -8, 8 };

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
	game.Material = 0;
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

void MakeMove(Move move) {
	
	PieceType pt = game.Squares[move.To];
	game.Material -= MaterialMatrix[pt >> 4][pt & 7];

	game.Squares[move.To] = game.Squares[move.From];
	game.Squares[move.From] = NOPIECE;

	//resetting en passant every move
	game.State &= ~15;

	switch (move.MoveInfo)
	{
	case PromotionQueen:
		game.Squares[move.To] = QUEEN | game.Side;
		game.Material += MaterialMatrix[game.Side >> 4][QUEEN + 6];
		break;
	case PromotionRook:
		game.Squares[move.To] = ROOK | game.Side;
		game.Material += MaterialMatrix[game.Side >> 4][ROOK + 6];
		break;
	case PromotionBishop:
		game.Squares[move.To] = BISHOP | game.Side;
		game.Material += MaterialMatrix[game.Side >> 4][BISHOP + 6];
		break;
	case PromotionKnight:
		game.Squares[move.To] = KNIGHT | game.Side;
		game.Material += MaterialMatrix[game.Side >> 4][KNIGHT + 6];
		break;
	case KingMove:
		game.KingSquares[game.Side >> 4] = move.To;
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
		game.KingSquares[game.Side >> 4] = move.To;
		game.Squares[7 + CastlesOffset[game.Side >> 4]] = NOPIECE;
		game.Squares[5 + CastlesOffset[game.Side >> 4]] = ROOK | game.Side;
		break;
	case CastleLong:
		game.KingSquares[game.Side >> 4] = move.To;
		game.Squares[0 + CastlesOffset[game.Side >> 4]] = NOPIECE;
		game.Squares[3 + CastlesOffset[game.Side >> 4]] = ROOK | game.Side;
		break;
	case EnPassant:
		game.State |= ((move.From & 7) + 1); //Sets the file. a to h file is 1 to 8.
		break;
	case EnPassantCapture:
		game.Squares[move.To + _behind[game.Side >> 4]] = NOPIECE;
		game.Material += MaterialMatrix[game.Side >> 4][PAWN];
		break;
	default:
		break;
	}
	game.Side ^= 24;
}

void UnMakeMove(Move move, PieceType capture, GameState prevGameState) {

	game.Material += MaterialMatrix[capture >> 4][capture & 7];

	game.Squares[move.From] = game.Squares[move.To];
	game.Squares[move.To] = capture;
	int otherSide = game.Side ^ 24;
	switch (move.MoveInfo)
	{
	case PromotionQueen:
		game.Material -= MaterialMatrix[otherSide >> 4][QUEEN + 6];
		game.Squares[move.From] = PAWN | otherSide;
		break;
	case PromotionRook:
		game.Material -= MaterialMatrix[otherSide >> 4][ROOK + 6];
		game.Squares[move.From] = PAWN | otherSide;
		break;
	case PromotionBishop:
		game.Material -= MaterialMatrix[otherSide >> 4][BISHOP + 6];
		game.Squares[move.From] = PAWN | otherSide;
		break;
	case PromotionKnight:
		game.Material -= MaterialMatrix[otherSide >> 4][KNIGHT + 6];
		game.Squares[move.From] = PAWN | otherSide;
		break;
	case KingMove:
		game.KingSquares[otherSide >> 4] = move.From;
		break;
	case CastleShort:
		game.KingSquares[otherSide >> 4] = move.From;
		game.Squares[5 + CastlesOffset[otherSide >> 4]] = NOPIECE;
		game.Squares[7 + CastlesOffset[otherSide >> 4]] = ROOK | otherSide;
		break;
	case CastleLong:
		game.KingSquares[otherSide >> 4] = move.From;
		game.Squares[3 + CastlesOffset[otherSide >> 4]] = NOPIECE;
		game.Squares[0 + CastlesOffset[otherSide >> 4]] = ROOK | otherSide;
		break;
	case EnPassantCapture:
		game.Squares[move.To + _behind[otherSide >> 4]] = PAWN | game.Side;
		game.Material -= MaterialMatrix[otherSide >> 4][PAWN];
		break;
	default:
		break;
	}
	game.State = prevGameState;
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
				for (int s = 1; s <= rayLength; s++)
				{
					int toSquare = PieceTypeSquareRaysPatterns[pat][i][r][s];
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

void CreateMove(int fromSquare, int toSquare, MoveInfo moveInfo) {
	PieceType capture = game.Squares[toSquare];
	GameState prevGameState = game.State;
	Move move;
	move.From = fromSquare;
	move.To = toSquare;
	move.MoveInfo = moveInfo;
	MakeMove(move);
	
	int  kingSquare = game.KingSquares[(game.Side ^ 24) >> 4];
	bool legal = !SquareAttacked(kingSquare, game.Side);
	if (legal)
		game.MovesBuffer[game.MovesBufferLength++] = move;

	UnMakeMove(move, capture, prevGameState);
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
					for (int s = 1; s <= rayLength; s++)
					{
						int toSquare = PieceTypeSquareRaysPatterns[pat][i][r][s];
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

			//en passant
			//castling
		}
	}
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
	//unsigned int size = sizeof(Move);
	Move * localMoves = malloc(game.MovesBufferLength * MOVESIZE);
	memcpy(localMoves, game.MovesBuffer, game.MovesBufferLength * MOVESIZE);
	int count = game.MovesBufferLength;
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
		MakeMove(move);
		nodeCount += Perft(depth - 1);
		UnMakeMove(move, capture, prevGameState);
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

void InitMaterial() {
	game.Material = 0;
	for (int i = 0; i < 64; i++)
	{
		PieceType pt = game.Squares[i] & 7;
		int colorSide = (game.Squares[i] & (WHITE | BLACK)) >> 4;
		game.Material += MaterialMatrix[colorSide][pt];
	}
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
	InitMaterial();
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
				{fenBuffer[index++] = '0' + emptyCount;
				file--;
				}
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

int MakePlayerMove(char * sMove) {
	Move move = parseMove(sMove, 0);
	Move moves[100];
	int length = ValidMoves(moves);
	for (int i = 0; i < length; i++)
	{
		if (moves[i].From == move.From && moves[i].To == move.To) {
			MakeMove(moves[i]);
			return 1;
		}
	}
	return 0;
}

int main() {
	InitGame();
	char s = 0;
	while (s != 'q')
	{
		PrintGame();
		printf("m: make move\n");
		printf("c: cpu move\n");
		printf("t: run tests\n");
		printf("q: quit\n");
		scanf_s(" %c", &s, 1);
		system("@cls||clear");
		switch (s)
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

