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
enum Color side = White;

int perftCount;
int KingSquares[2];
PieceType Squares[64];

bool CastleKingSideEnabled[2];
bool CastleQueenSideEnabled[2];

void InitPiece(int file, int rank, enum PieceType type, enum Color color) {
	Squares[rank * 8 + file] = type | color;
}

void dump() {

	PrintGame();
	char str1[10];
	fgets(str1, 5, stdin);
}

void InitGame() {
	for (int i = 0; i < 64; i++)
	{
		Squares[i] = NoPiece;
	}

	InitPiece(0, 0, Rook, White);
	InitPiece(1, 0, Knight, White);
	InitPiece(2, 0, Bishop, White);
	InitPiece(3, 0, Queen, White);
	InitPiece(4, 0, King, White);
	KingSquares[0] = 4;
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
	KingSquares[1] = 60;
	InitPiece(5, 7, Bishop, Black);
	InitPiece(6, 7, Knight, Black);
	InitPiece(7, 7, Rook, Black);

	for (int i = 0; i < 8; i++)
		InitPiece(i, 6, Pawn, Black);
	side = White;

	CastleKingSideEnabled[0] = true;
	CastleKingSideEnabled[1] = true;
	CastleQueenSideEnabled[0] = true;
	CastleQueenSideEnabled[1] = true;
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
			PieceType piece = Squares[r * 8 + f];
			char c = PieceChar(piece);
			printf("| %c ", c);
		}
		printf("|\n  ---------------------------------\n");
	}
	printf("    a   b   c   d   e   f   g   h  \n");
}

void MakeMove(Move move) {

	Squares[move.To] = Squares[move.From];
	Squares[move.From] = NoPiece;
	int castleBlackOffset = side == White ? 0 : 56;

	switch (move.MoveInfo)
	{
	case Promotion:
		Squares[move.To] = Queen | side;
		break;
	case KingMove:
		KingSquares[side >> 4] = move.To;
		break;
	case CastleShort:
		KingSquares[side >> 4] = move.To;
		Squares[7 + castleBlackOffset] = NoPiece;
		Squares[5 + castleBlackOffset] = Rook | side;
		//todo: test
		break;
	case CastleLong:
		KingSquares[side >> 4] = move.To;
		Squares[0 + castleBlackOffset] = NoPiece;
		Squares[3 + castleBlackOffset] = Rook | side;
		//todo: test
		break;
	default:
		break;
	}

	//castling, short long
	//en passant
}

void UnMakeMove(Move move, PieceType capture) {

	Squares[move.From] = Squares[move.To];
	Squares[move.To] = capture;
	int otherSide = side ^ 24;
	int castleBlackOffset = otherSide == White ? 0 : 56;
	switch (move.MoveInfo)
	{
	case Promotion:
		Squares[move.From] = Pawn | otherSide;
		break;
	case KingMove:
		KingSquares[otherSide >> 4] = move.From;
		break;
	case CastleShort:
		KingSquares[otherSide >> 4] = move.From;
		Squares[5 + castleBlackOffset] = NoPiece;
		Squares[7 + castleBlackOffset] = Rook | otherSide;
		//todo: test
		break;
	case CastleLong:
		KingSquares[otherSide >> 4] = move.From;
		Squares[3 + castleBlackOffset] = NoPiece;
		Squares[0 + castleBlackOffset] = Rook | otherSide;
		//todo: test
		break;
	default:
		break;
	}

	/*if (move->From->Piece.Type == King)
	KingSquares[side] = move->From;
	move->From->Piece = move->To->Piece;
	move->To->Piece = move->Capture;
	if (move->Promotion)
	move->From->Piece.Type = Pawn;*/
}

bool SquareAttacked(int square) {
	for (int i = 0; i < 64; i++)
	{
		PieceType pieceType = Squares[i];
		PieceType color = pieceType & (Black | White) ^ 24;

		if (color != side)
			continue;
		PieceType pt = pieceType & 7;
		switch (pt)
		{
		case Pawn:
		{
			int captPat = side & White ? 5 : 3;
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
					if (Squares[toSquare] > NoPiece)
						break;
				}
			}
			break;
		}
		}

	}
	return false;
}

void PerftMove(int fromSquare, int toSquare, int depth, MoveInfo moveInfo) {
	PieceType capture = Squares[toSquare];
	if ((capture & 7) == King)
		return;
	Move move;
	move.From = fromSquare;
	move.To = toSquare;
	move.MoveInfo = moveInfo;
	MakeMove(move);
	
	int  kingSquare = KingSquares[side >> 4];
	move.Legal = !SquareAttacked(kingSquare);
	side ^= 24;
	if (move.Legal)
		Perft(depth - 1);

	UnMakeMove(move, capture);
	side ^= 24;
}

void Perft(int depth) {
	if (depth == 0)
	{
		perftCount++;
		return;
	}
	for (int i = 0; i < 64; i++)
	{
		PieceType pieceType = Squares[i];
		if (pieceType != NoPiece && (pieceType & side)) {
			PieceType pt = pieceType & 7;
			switch (pt)
			{
			case Pawn:
			{
				int pat = side & White ? 2 : 4;
				int pawnPatLength = PieceTypeSquarePatterns[pat][i][0];
				for (int pp = 1; pp <= pawnPatLength; pp++)
				{
					int toSquare = PieceTypeSquarePatterns[pat][i][pp];
					//tillåt inte två drag när en pjäs står ivägen
					if (pp == 2) {
						int overSqr = PieceTypeSquarePatterns[pat][i][1];
						if (Squares[overSqr] != NoPiece)
							break;
						//en passant here
					}
					MoveInfo info = (toSquare < 8 || toSquare > 55) ? Promotion : 0;
					if (Squares[toSquare] == NoPiece) {
						PerftMove(i, toSquare, depth, info);
					}
				}

				int captPat = side & White ? 3 : 5;
				int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
				for (int pc = 1; pc <= pawnCapPatLength; pc++)
				{
					int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
					MoveInfo info = (toSquare < 8 || toSquare > 55) ? Promotion : 0;
					//must be a piece of opposite color.
					if (Squares[toSquare] & (side ^ 24)) {
						PerftMove(i, toSquare, depth, info);
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
					if (!(Squares[toSquare] & side)) {
						PerftMove(i, toSquare, depth, 0);
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
					if (!(Squares[toSquare] & side)) {
						PerftMove(i, toSquare, depth, KingMove);
					}
				}

				int castleBlackOffset = side == White ? 0 : 56;
				if (i == castleBlackOffset + 4) { //King on origin pos
					if (CastleKingSideEnabled[side >> 4]) {
						if ((Squares[castleBlackOffset + 4] & 7) == Rook &&
							Squares[castleBlackOffset + 5] == NoPiece &&
							Squares[castleBlackOffset + 6] == NoPiece)
						{
							if (!SquareAttacked(5 + castleBlackOffset) && !SquareAttacked(6 + castleBlackOffset))
								PerftMove(i, 6 + castleBlackOffset, depth, CastleShort);
						}
						//King on e file (4) ?
						//Rook on h file (7)
						//No pieces on f or g (5 or 6)
						//Not Squares attacked file 4, 5 or 6
					}
					if (CastleQueenSideEnabled[side >> 4]) {
						if ((Squares[castleBlackOffset - 5] & 7) == Rook &&
							Squares[castleBlackOffset + 1] == NoPiece &&
							Squares[castleBlackOffset + 2] == NoPiece &&
							Squares[castleBlackOffset + 3] == NoPiece)
						{
							if (!SquareAttacked(2 + castleBlackOffset) && !SquareAttacked(3 + castleBlackOffset))
								PerftMove(i, 2 + castleBlackOffset, depth, CastleLong);
						}
						//King on e file (4) ?
						//Rook on a file (0)
						//No pieces on c or d (2 or 3)
						//No Squares attacked file 4, 3 or 2 //mest prestanda. händer inte ofta
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
						PieceType toPiece = Squares[toSquare];
						if (toPiece != NoPiece) {
							if (!(toPiece & side)) {
								PerftMove(i, toSquare, depth, 0);
							}
							break;
						}
						else {
							PerftMove(i, toSquare, depth, 0);
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
		Squares[i] = NoPiece;
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
			Squares[rank * 8 + file] = parsePieceType(c);
			file++;
		}
	}

	index++;
	side = parseSide(fen[index]);
	index++;
	index++;

	CastleKingSideEnabled[0] = false;
	CastleQueenSideEnabled[0] = false;
	CastleKingSideEnabled[1] = false;
	CastleQueenSideEnabled[1] = false;
	while (fen[index] != ' ')
	{
		switch (fen[index])
		{
		case 'K': CastleKingSideEnabled[0] = true;
			break;
		case 'Q': CastleQueenSideEnabled[0] = true;
			break;
		case 'k': CastleKingSideEnabled[1] = true;
			break;
		case 'q': CastleQueenSideEnabled[1] = true;
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
			while (Squares[rank * 8 + file] == NoPiece && file < 8)
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
				fenBuffer[index++] = PieceChar(Squares[rank * 8 + file]);
			}
		}
		if (rank > 0)
			fenBuffer[index++] = '/';
	}
	fenBuffer[index++] = ' ';
	fenBuffer[index++] = side == White ? 'w' : 'b';
	fenBuffer[index++] = ' ';
	if (CastleKingSideEnabled[0]) fenBuffer[index++] = 'K';
	if (CastleQueenSideEnabled[0]) fenBuffer[index++] = 'Q';
	if (CastleKingSideEnabled[1]) fenBuffer[index++] = 'k';
	if (CastleQueenSideEnabled[1]) fenBuffer[index++] = 'q';
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
		printf("p: perft 5\n");
		printf("q: quit\n");
		scanf_s(" %c", &s);
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

