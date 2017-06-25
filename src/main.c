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

int _perftCount;
PerftResult _perftResult;
//Game game;
char _side = WHITE;

//Keeps track of kings. Since kings square is checked against attack in legal evaluation so often this helps performance.
int _kingSquares[2];

PieceType _squares[64];

int _movesBufferLength = 0;
//Buffer for move generation
//Assuming a position can never generate more than 100 moves.
Move _movesBuffer[100];

GameState _gameState;


//one square behind another square, subtract by 8 for white(0) and add 8 for black(1).
int _behind[] = { -8, 8 };

void InitPiece(int file, int rank, enum PieceType type, enum Color color) {
	_squares[rank * 8 + file] = type | color;
}

void InitGame() {
	for (int i = 0; i < 64; i++)
		_squares[i] = NOPIECE;

	InitPiece(0, 0, ROOK, WHITE);
	InitPiece(1, 0, KNIGHT, WHITE);
	InitPiece(2, 0, BISHOP, WHITE);
	InitPiece(3, 0, QUEEN, WHITE);
	InitPiece(4, 0, KING, WHITE);
	_kingSquares[0] = 4;
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
	_kingSquares[1] = 60;
	InitPiece(5, 7, BISHOP, BLACK);
	InitPiece(6, 7, KNIGHT, BLACK);
	InitPiece(7, 7, ROOK, BLACK);

	for (int i = 0; i < 8; i++)
		InitPiece(i, 6, PAWN, BLACK);
	_side = WHITE;

	_gameState = WhiteCanCastleLong | WhiteCanCastleShort | BlackCanCastleLong | BlackCanCastleShort;
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
			PieceType piece = _squares[r * 8 + f];
			char c = PieceChar(piece);
			printf("| %c ", c);
		}
		printf("|\n  ---------------------------------\n");
	}
	printf("    a   b   c   d   e   f   g   h  \n");
}

void MakeMove(Move move) {

	_squares[move.To] = _squares[move.From];
	_squares[move.From] = NOPIECE;
	int castleBlackOffset = _side == WHITE ? 0 : 56;

	//resetting en passant every move
	_gameState &= ~15;

	switch (move.MoveInfo)
	{
	case PromotionQueen:
		_squares[move.To] = QUEEN | _side;
		break;
	case PromotionRook:
		_squares[move.To] = ROOK | _side;
		break;
	case PromotionBishop:
		_squares[move.To] = BISHOP | _side;
		break;
	case PromotionKnight:
		_squares[move.To] = KNIGHT | _side;
		break;
	case KingMove:
		_kingSquares[_side >> 4] = move.To;
		break;
	case RookMove:
		switch (move.From)
		{
		case 0:
			_gameState &= ~WhiteCanCastleLong;
			break;
		case 7:
			_gameState &= ~WhiteCanCastleShort;
			break;
		case 56:
			_gameState &= ~BlackCanCastleLong;
			break;
		case 63:
			_gameState &= ~BlackCanCastleShort;
			break;
		default:
			break;
		}
		break;
	case CastleShort:
		_kingSquares[_side >> 4] = move.To;
		_squares[7 + castleBlackOffset] = NOPIECE;
		_squares[5 + castleBlackOffset] = ROOK | _side;
		break;
	case CastleLong:
		_kingSquares[_side >> 4] = move.To;
		_squares[0 + castleBlackOffset] = NOPIECE;
		_squares[3 + castleBlackOffset] = ROOK | _side;
		break;
	case EnPassant:
		_gameState |= ((move.From & 7) + 1); //Sets the file. a to h file is 1 to 8.
		break;
	case EnPassantCapture:
		_squares[move.To + _behind[_side >> 4]] = NOPIECE;
		break;
	default:
		break;
	}
	//en passant
}

void UnMakeMove(Move move, PieceType capture, GameState prevGameState) {
	_squares[move.From] = _squares[move.To];
	_squares[move.To] = capture;
	int otherSide = _side ^ 24;
	int castleBlackOffset = otherSide == WHITE ? 0 : 56;
	switch (move.MoveInfo)
	{

	case PromotionQueen:
	case PromotionRook:
	case PromotionBishop:
	case PromotionKnight:
		_squares[move.From] = PAWN | otherSide;
		break;
	case KingMove:
		_kingSquares[otherSide >> 4] = move.From;
		break;
	case CastleShort:
		_kingSquares[otherSide >> 4] = move.From;
		_squares[5 + castleBlackOffset] = NOPIECE;
		_squares[7 + castleBlackOffset] = ROOK | otherSide;
		break;
	case CastleLong:
		_kingSquares[otherSide >> 4] = move.From;
		_squares[3 + castleBlackOffset] = NOPIECE;
		_squares[0 + castleBlackOffset] = ROOK | otherSide;
		break;
	case EnPassantCapture:
		_squares[move.To + _behind[otherSide >> 4]] = PAWN | _side;
		break;
	default:
		break;
	}
	_gameState = prevGameState;
}

bool SquareAttacked(int square) {
	for (int i = 0; i < 64; i++)
	{
		PieceType pieceType = _squares[i];
		PieceType color = pieceType & (BLACK | WHITE) ^ 24;

		if (color != _side)
			continue;
		PieceType pt = pieceType & 7;
		switch (pt)
		{
		case PAWN:
		{
			int captPat = _side & WHITE ? 5 : 3;
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
					if (_squares[toSquare] > NOPIECE)
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
	PieceType capture = _squares[toSquare];
	GameState prevGameState = _gameState;
	Move move;
	move.From = fromSquare;
	move.To = toSquare;
	move.MoveInfo = moveInfo;
	MakeMove(move);
	
	int  kingSquare = _kingSquares[_side >> 4];
	bool legal = !SquareAttacked(kingSquare);
	_side ^= 24;
	if (legal)
		_movesBuffer[_movesBufferLength++] = move;

	UnMakeMove(move, capture, prevGameState);
	_side ^= 24;
}

void CreateMoves() {
	_movesBufferLength = 0;
	for (int i = 0; i < 64; i++)
	{
		PieceType pieceType = _squares[i];
		if (pieceType != NOPIECE && (pieceType & _side)) {
			PieceType pt = pieceType & 7;
			switch (pt)
			{
			case PAWN:
			{
				int pat = _side & WHITE ? 2 : 4; //todo: optimize
				int pawnPatLength = PieceTypeSquarePatterns[pat][i][0];
				for (int pp = 1; pp <= pawnPatLength; pp++)
				{
					int toSquare = PieceTypeSquarePatterns[pat][i][pp];
					if (_squares[toSquare] != NOPIECE)
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

				int captPat = _side & WHITE ? 3 : 5; //todo: optimize
				int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
				for (int pc = 1; pc <= pawnCapPatLength; pc++)
				{
					int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
					//Must be a piece of opposite color.
					if (_squares[toSquare] & (_side ^ 24))
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
						int enpFile = (_gameState & 15) - 1;
						if (enpFile > -1) {
							int toFile = toSquare & 7;
							int toRank = toSquare >> 3;
							if (toFile == enpFile && toRank == (_side & WHITE ? 5 : 2)) { //todo: optimize
								CreateMove(i, toSquare, EnPassantCapture);
							}
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
					if (!(_squares[toSquare] & _side)) {
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
					if (!(_squares[toSquare] & _side)) {
						CreateMove(i, toSquare, KingMove);
					}
				}

				int castleBlackOffset = _side == WHITE ? 0 : 56;
				if (i == castleBlackOffset + 4) { //King on origin pos
					if ((_side & WHITE && _gameState & WhiteCanCastleShort) || (_side & BLACK && _gameState & BlackCanCastleShort)) {
						if ((_squares[castleBlackOffset + 7] & 7) == ROOK &&
							_squares[castleBlackOffset + 5] == NOPIECE &&
							_squares[castleBlackOffset + 6] == NOPIECE)
						{
							if (!SquareAttacked(5 + castleBlackOffset) && !SquareAttacked(4 + castleBlackOffset))
								CreateMove(i, 6 + castleBlackOffset, CastleShort);
						}
					}
					if ((_side & WHITE && _gameState & WhiteCanCastleLong) || (_side & BLACK && _gameState & BlackCanCastleLong)) {
						if ((_squares[castleBlackOffset] & 7) == ROOK &&
							_squares[castleBlackOffset + 1] == NOPIECE &&
							_squares[castleBlackOffset + 2] == NOPIECE &&
							_squares[castleBlackOffset + 3] == NOPIECE)
						{
							if (!SquareAttacked(4 + castleBlackOffset) && !SquareAttacked(3 + castleBlackOffset))
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
						PieceType toPiece = _squares[toSquare];
						MoveInfo moveInfo = pt == ROOK ? RookMove : PlainMove;

						if (toPiece != NOPIECE) {
							if (!(toPiece & _side)) {
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
	if (_movesBufferLength == 0)
		return nodeCount;
	//unsigned int size = sizeof(Move);
	Move * localMoves = malloc(_movesBufferLength * MOVESIZE);
	memcpy(localMoves, _movesBuffer, _movesBufferLength * MOVESIZE);
	int count = _movesBufferLength;
	for (int i = 0; i < count; i++)
	{
		Move move = localMoves[i];
		PieceType capture = _squares[move.To];
		
		if (depth == 1) {
			if (capture != NOPIECE)
				_perftResult.Captures++;
			if (move.MoveInfo == EnPassantCapture)
				_perftResult.Captures++;
			if (move.MoveInfo == CastleLong || move.MoveInfo == CastleShort)
				_perftResult.Castles++;
			if (move.MoveInfo >= PromotionQueen && move.MoveInfo <= PromotionKnight)
				_perftResult.Promotions++;
			if (move.MoveInfo == EnPassantCapture)
				_perftResult.Enpassants++;
		}

		GameState prevGameState = _gameState;
		MakeMove(move);
		_side ^= 24;
		nodeCount += Perft(depth - 1);
		UnMakeMove(move, capture, prevGameState);
		_side ^= 24;
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

void ReadFen(char * fen) {
	//rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
	for (size_t i = 0; i < 64; i++)
		_squares[i] = NOPIECE;
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
			_squares[rank * 8 + file] = parsePieceType(c);
			file++;
		}
	}

	index++;
	_side = parseSide(fen[index]);
	index++;
	index++;
	_gameState = 0;
	while (fen[index] != ' ')
	{
		switch (fen[index])
		{
		case 'K': _gameState |= WhiteCanCastleShort;
			break;
		case 'Q': _gameState |= WhiteCanCastleLong;
			break;
		case 'k': _gameState |= BlackCanCastleShort;
			break;
		case 'q': _gameState |= BlackCanCastleLong;
			break;
		default:
			break;
		}
		index++;
	}
	index++;
	char enpFile = fen[index] - 'a';
	if (enpFile >= 0 && enpFile <= 8)
		_gameState |= (enpFile + 1);
	//todo enpassant file
	//todo counters
}

void WriteFen(char * fenBuffer) {
	int index = 0;
	for (int rank = 8 - 1; rank >= 0; rank--)
	{
		for (int file = 0; file < 8; file++)
		{
			int emptyCount = 0;
			while (_squares[rank * 8 + file] == NOPIECE && file < 8)
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
				fenBuffer[index++] = PieceChar(_squares[rank * 8 + file]);
			}
		}
		if (rank > 0)
			fenBuffer[index++] = '/';
	}
	fenBuffer[index++] = ' ';
	fenBuffer[index++] = _side == WHITE ? 'w' : 'b';
	fenBuffer[index++] = ' ';
	if (_gameState & WhiteCanCastleShort) fenBuffer[index++] = 'K';
	if (_gameState & WhiteCanCastleLong) fenBuffer[index++] = 'Q';
	if (_gameState & BlackCanCastleShort) fenBuffer[index++] = 'k';
	if (_gameState & BlackCanCastleLong) fenBuffer[index++] = 'q';
	fenBuffer[index++] = ' ';
	char enPassantFile = (_gameState & 15) + '0';
	if (enPassantFile == '0')
		enPassantFile = '-';
	fenBuffer[index++] = enPassantFile;
	fenBuffer[index] = '\0';
}

int ValidMoves(Move * moves) {
	CreateMoves();
	if (_movesBufferLength == 0)
		return 0;

	memcpy(moves, _movesBuffer, _movesBufferLength * MOVESIZE);
	return _movesBufferLength;
}

int MakePlayerMove(Move move) {
	Move moves[100];
	int length = ValidMoves(moves);
	for (int i = 0; i < length; i++)
	{
		if (moves[i].From == move.From && moves[i].To == move.To) {
			MakeMove(moves[i]);
			_side ^= 24;
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

