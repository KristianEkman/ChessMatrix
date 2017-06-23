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

//Game game;
enum Color _side = White;
int perftCount;

//Keeps track of kings. Since kings square is checked against attack in legal evaluation so often.
int _kingSquares[2];

PieceType _squares[64];

int _movesBufferLength = 0;
//Buffer for move generation
//Assuming a position can never generate more than 100 moves.
Move _movesBuffer[100];

//For faster undo
GameState _gameState;

void InitPiece(int file, int rank, enum PieceType type, enum Color color) {
	_squares[rank * 8 + file] = type | color;
}

void InitGame() {
	for (int i = 0; i < 64; i++)
		_squares[i] = NoPiece;

	InitPiece(0, 0, Rook, White);
	InitPiece(1, 0, Knight, White);
	InitPiece(2, 0, Bishop, White);
	InitPiece(3, 0, Queen, White);
	InitPiece(4, 0, King, White);
	_kingSquares[0] = 4;
	InitPiece(5, 0, Bishop, White);
	InitPiece(6, 0, Knight, White);
	InitPiece(7, 0, Rook, White);

	for (int i = 0; i < 8; i++)
		InitPiece(i, 1, Pawn, White);

	InitPiece(0, 7, Rook, Black);
	InitPiece(1, 7, Knight, Black);
	InitPiece(2, 7, Bishop, Black);
	InitPiece(3, 7, Queen, Black);
	InitPiece(4, 7, King, Black);
	_kingSquares[1] = 60;
	InitPiece(5, 7, Bishop, Black);
	InitPiece(6, 7, Knight, Black);
	InitPiece(7, 7, Rook, Black);

	for (int i = 0; i < 8; i++)
		InitPiece(i, 6, Pawn, Black);
	_side = White;

	_gameState = WhiteCanCastleLong | WhiteCanCastleShort | BlackCanCastleLong | BlackCanCastleShort;
}

char PieceChar(PieceType pieceType) {
	PieceType color = pieceType & (Black | White);
	PieceType pt = pieceType & 7;
	switch (pt)
	{
	case Pawn:
		return color == White ? 'P' : 'p';
	case Rook:
		return color == White ? 'R' : 'r';
	case Knight:
		return color == White ? 'N' : 'n';
	case Bishop:
		return color == White ? 'B' : 'b';
	case Queen:
		return color == White ? 'Q' : 'q';
	case King:
		return color == White ? 'K' : 'k';
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
	_squares[move.From] = NoPiece;
	int castleBlackOffset = _side == White ? 0 : 56;

	switch (move.MoveInfo)
	{
	case Promotion:
		_squares[move.To] = Queen | _side;
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
		_squares[7 + castleBlackOffset] = NoPiece;
		_squares[5 + castleBlackOffset] = Rook | _side;
		break;
	case CastleLong:
		_kingSquares[_side >> 4] = move.To;
		_squares[0 + castleBlackOffset] = NoPiece;
		_squares[3 + castleBlackOffset] = Rook | _side;
		break;
	default:
		break;
	}
	//en passant
}

void UnMakeMove(Move move, PieceType capture, GameState _prevGameState) {
	_squares[move.From] = _squares[move.To];
	_squares[move.To] = capture;
	int otherSide = _side ^ 24;
	int castleBlackOffset = otherSide == White ? 0 : 56;
	switch (move.MoveInfo)
	{
	case Promotion:
		_squares[move.From] = Pawn | otherSide;
		break;
	case KingMove:
		_kingSquares[otherSide >> 4] = move.From;
		break;
	case CastleShort:
		_kingSquares[otherSide >> 4] = move.From;
		_squares[5 + castleBlackOffset] = NoPiece;
		_squares[7 + castleBlackOffset] = Rook | otherSide;
		break;
	case CastleLong:
		_kingSquares[otherSide >> 4] = move.From;
		_squares[3 + castleBlackOffset] = NoPiece;
		_squares[0 + castleBlackOffset] = Rook | otherSide;
		break;
	default:
		break;
	}
	_gameState = _prevGameState;
	//en passant

}

bool SquareAttacked(int square) {
	for (int i = 0; i < 64; i++)
	{
		PieceType pieceType = _squares[i];
		PieceType color = pieceType & (Black | White) ^ 24;

		if (color != _side)
			continue;
		PieceType pt = pieceType & 7;
		switch (pt)
		{
		case Pawn:
		{
			int captPat = _side & White ? 5 : 3;
			int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
			for (int pc = 1; pc <= pawnCapPatLength; pc++)
			{
				int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
				if (toSquare == square)
					return true;
			}
			break;
		}
		case Knight:
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
		case King:
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
			int raysCount = PieceTypeSquareRaysPatterns[pat][i][0][0];
			for (int r = 1; r <= raysCount; r++)
			{
				int rayLength = PieceTypeSquareRaysPatterns[pat][i][r][0];
				for (int s = 1; s <= rayLength; s++)
				{
					int toSquare = PieceTypeSquareRaysPatterns[pat][i][r][s];
					if (toSquare == square)
						return true;
					if (_squares[toSquare] > NoPiece)
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
	if ((capture & 7) == King)
		return;
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
		if (pieceType != NoPiece && (pieceType & _side)) {
			PieceType pt = pieceType & 7;
			switch (pt)
			{
			case Pawn:
			{
				int pat = _side & White ? 2 : 4;
				int pawnPatLength = PieceTypeSquarePatterns[pat][i][0];
				for (int pp = 1; pp <= pawnPatLength; pp++)
				{
					int toSquare = PieceTypeSquarePatterns[pat][i][pp];
					//tillåt inte två drag när en pjäs står ivägen
					if (pp == 2) {
						int overSqr = PieceTypeSquarePatterns[pat][i][1];
						if (_squares[overSqr] != NoPiece)
							break;
						//en passant här
					}
					MoveInfo info = (toSquare < 8 || toSquare > 55) ? Promotion : 0;
					if (_squares[toSquare] == NoPiece) {
						CreateMove(i, toSquare, info);
					}
				}

				int captPat = _side & White ? 3 : 5;
				int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
				for (int pc = 1; pc <= pawnCapPatLength; pc++)
				{
					int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
					MoveInfo info = (toSquare < 8 || toSquare > 55) ? Promotion : 0;
					//must be a piece of opposite color.
					if (_squares[toSquare] & (_side ^ 24)) {
						CreateMove(i, toSquare, info);
					}
				}
				break;
			}
			case Knight:
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
			case King:
			{
				int length = PieceTypeSquarePatterns[1][i][0];
				for (int p = 1; p <= length; p++)
				{
					int toSquare = PieceTypeSquarePatterns[1][i][p];
					if (!(_squares[toSquare] & _side)) {
						CreateMove(i, toSquare, KingMove);
					}
				}

				int castleBlackOffset = _side == White ? 0 : 56;
				if (i == castleBlackOffset + 4) { //King on origin pos
					if ((_side & White && _gameState & WhiteCanCastleShort) || (_side & Black && _gameState & BlackCanCastleShort)) {
						if ((_squares[castleBlackOffset + 4] & 7) == Rook &&
							_squares[castleBlackOffset + 5] == NoPiece &&
							_squares[castleBlackOffset + 6] == NoPiece)
						{
							if (!SquareAttacked(5 + castleBlackOffset) && !SquareAttacked(6 + castleBlackOffset))
								CreateMove(i, 6 + castleBlackOffset, CastleShort);
						}
					}
					if ((_side & White && _gameState & WhiteCanCastleLong) || (_side & Black && _gameState & BlackCanCastleLong)) {
						if ((_squares[castleBlackOffset - 5] & 7) == Rook &&
							_squares[castleBlackOffset + 1] == NoPiece &&
							_squares[castleBlackOffset + 2] == NoPiece &&
							_squares[castleBlackOffset + 3] == NoPiece)
						{
							if (!SquareAttacked(2 + castleBlackOffset) && !SquareAttacked(3 + castleBlackOffset))
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
						MoveInfo moveInfo = pt == Rook ? RookMove : PlainMove;

						if (toPiece != NoPiece) {
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
		return 1;
	int nodeCount = 0;
	CreateMoves();
	//unsigned int size = sizeof(Move);
	Move * localMoves = malloc(_movesBufferLength * 8);
	memcpy(localMoves, _movesBuffer, _movesBufferLength * 8);
	int count = _movesBufferLength;
	for (int i = 0; i < count; i++)
	{
		Move move = localMoves[i];
		PieceType capture = _squares[move.To];
		GameState prevGameState = _gameState;
		MakeMove(localMoves[i]);
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
	case 'p': return Pawn | Black;
	case 'r': return Rook | Black;
	case 'b': return Bishop | Black;
	case 'n': return Knight | Black;
	case 'q': return Queen | Black;
	case 'k': return King | Black;
	case 'P': return Pawn | White;
	case 'R': return Rook | White;
	case 'B': return Bishop | White;
	case 'N': return Knight | White;
	case 'Q': return Queen | White;
	case 'K': return King | White;
		
	default:
		return NoPiece;
	}
}

PieceType parseSide(char c) {
	switch (c)
	{
	case 'w': return White;
	case 'b': return Black;
	default:
		return NoPiece;
	}
}

void ReadFen(char * fen) {
	//rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
	for (size_t i = 0; i < 64; i++)
		_squares[i] = NoPiece;
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
			while (_squares[rank * 8 + file] == NoPiece && file < 8)
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
	fenBuffer[index++] = _side == White ? 'w' : 'b';
	fenBuffer[index++] = ' ';
	if (_gameState & WhiteCanCastleShort) fenBuffer[index++] = 'K';
	if (_gameState & WhiteCanCastleLong) fenBuffer[index++] = 'Q';
	if (_gameState & BlackCanCastleShort) fenBuffer[index++] = 'k';
	if (_gameState & BlackCanCastleLong) fenBuffer[index++] = 'q';
	fenBuffer[index] = '\0';
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
	//ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		
	return 0;
}

