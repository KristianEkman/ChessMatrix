#include <stdio.h>
#include <time.h>
#include "basic_structs.h"
#include "patterns.h"
#include "main.h"

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
	printf("---------------------------------\n");

	for (int r = 8 - 1; r >= 0; r--)
	{
		for (int f = 0; f < 8; f++)
		{
			PieceType piece = Squares[r * 8 + f];
			char c = PieceChar(piece);
			printf("| %c ", c);
		}
		printf("|\n---------------------------------\n");
	}
}

void MakeMove(Move move) {

	Squares[move.To] = Squares[move.From];
	Squares[move.From] = NoPiece;
	
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
		//todo

		break;
	CastleLong:
		KingSquares[side >> 4] = move.To;
		//todo
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

	switch (move.MoveInfo)
	{
	case Promotion:
		Squares[move.From] = Pawn | (side ^ 24);
		break;
	case KingMove:
		KingSquares[side >> 4] = move.From;
		break;
	case CastleShort:
		KingSquares[side >> 4] = move.From;
		//todo
		break;
	CastleLong:
		KingSquares[side >> 4] = move.From;
		//todo
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
		}}
		return false;
	}
}

void PerftMove(int fromSquare, int toSquare, int depth, MoveInfo moveInfo) {
	PieceType capture = Squares[toSquare];
	if ((capture & 7) == King)
		return;
	Move move;
	move.From = fromSquare;
	move.To = toSquare;
	move.MoveInfo = moveInfo;
	//dump();
	MakeMove(move);
//	dump();
//	printf("%s", "-----\n");

	int  kingSquare = KingSquares[side];
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

				int castleBlackOffset = side == White ? 0 :56;
				if (KingSquares[side >> 4] == castleBlackOffset + 4) {
					if (CastleKingSideEnabled[side >> 4]) {
						if (i == 4 + castleBlackOffset &&
							Squares[5 + castleBlackOffset] == NoPiece &&
							Squares[6 + castleBlackOffset] == NoPiece)
						{
							if (!SquareAttacked(5 + castleBlackOffset) && !SquareAttacked(6 + castleBlackOffset))
								PerftMove(i, 6 + castleBlackOffset, depth, KingMove | CastleShort);
						}
						//King on e file (4) ?
						//Rook on h file (7)
						//No pieces on f or g (5 or 6)
						//Not Squares attacked file 4, 5 or 6
					}
					if (CastleQueenSideEnabled[side >> 4]) {
						if (i == 4 + castleBlackOffset &&
							Squares[2 + castleBlackOffset] == NoPiece &&
							Squares[3 + castleBlackOffset] == NoPiece)
						{
							if (!SquareAttacked(2 + castleBlackOffset) && !SquareAttacked(3 + castleBlackOffset))
								PerftMove(i, 2 + castleBlackOffset, depth, KingMove | CastleLong);
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

int main() {

	InitGame();
	PrintGame();

	for (size_t i = 0; i < 2; i++)
	{
		perftCount = 0;
		clock_t start = clock();
		Perft(5);
		clock_t stop = clock();
		float secs = (float)(stop - start) / CLOCKS_PER_SEC;
		printf("%.2fs\n", secs);
		printf("%dk moves\n", perftCount / 1000);
		printf("%.2fk moves/s\n", perftCount / (1000 * secs));

		PrintGame();
	}


	return 0;
}

