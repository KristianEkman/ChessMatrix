#include <stdio.h>
#include <time.h>
#include "basic_structs.h"
#include "patterns.h"

Game game;
Move psuedoMoves[50];
int psuedoMoveCount = 0;

Move moves[50];
int movesCount = 0;
enum Color side = White;

int perftCount;
void Perft(int depth);
void PrintGame();
int KingSquares[2];

void InitPiece(int file, int rank, enum PieceType type, enum Color color) {
	game.Squares[rank * 8 + file] = type | color;
}

void dump() {

	PrintGame();
	char str1[10];
	fgets(str1, 5, stdin);
}

void InitGame() {
	for (int i = 0; i < 64; i++)
	{
		game.Squares[i] = NoPiece;
	}

	InitPiece(0, 0, Rook, White);
	InitPiece(1, 0, Knight, White);
	InitPiece(2, 0, Bishop, White);
	InitPiece(3, 0, Queen, White);
	InitPiece(4, 0, King, White);
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
	InitPiece(5, 7, Bishop, Black);
	InitPiece(6, 7, Knight, Black);
	InitPiece(7, 7, Rook, Black);

	for (int i = 0; i < 8; i++)
		InitPiece(i, 6, Pawn, Black);
	side = White;
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
			PieceType piece = game.Squares[r * 8 + f];
			char c = PieceChar(piece);
			printf("| %c ", c);
		}
		printf("|\n---------------------------------\n");
	}
}

void MakeMove(Move move) {

	game.Squares[move.To] = game.Squares[move.From];
	game.Squares[move.From] = NoPiece;

	/*if (move->To->Piece.Type == King)
		KingSquares[side] = move->To;*/

	if (move.Promotion)
		game.Squares[move.To] = Queen | side;

	//castling, short long
	//en passant
}

void UnMakeMove(Move move, PieceType capture) {

	game.Squares[move.From] = game.Squares[move.To];
	game.Squares[move.To] = capture;

	if (move.Promotion)
		game.Squares[move.From] = Pawn | (side ^ 24);

	/*if (move->From->Piece.Type == King)
		KingSquares[side] = move->From;
	move->From->Piece = move->To->Piece;
	move->To->Piece = move->Capture;
	if (move->Promotion)
		move->From->Piece.Type = Pawn;*/
}

//
//bool KingAttacked() {
//	Square * kingSquare = KingSquares[side];
//	for (size_t i = 0; i < 64; i++)
//	{
//		PieceType pieceType = game.Squares[i];
//		if (piece.Type != NoPiece && piece.Color != side) {
//			if (piece.Type == Pawn) {
//				int * pawnCapts = GetPawnCapturePatterns(fromSquare, piece.Color);
//				int length = pawnCapts[0];
//				for (int i = 1; i <= length; i++)
//				{
//					if (pawnCapts[i] == kingSquare->Index) {
//						return true;
//					}
//				}
//			}
//
//			int * pattern = GetPatterns(fromSquare, piece);
//			int length = pattern[0];
//			for (int p = 1; p <= length; p++)
//			{
//				if (pattern[p] == kingSquare->Index) {
//					return true;
//				}
//			}
//
//			int ** rayPattens = GetRayPatterns(fromSquare, piece.Type);
//			int raysCount = rayPattens[0][0];
//			for (int r = 1; r <= raysCount; r++)
//			{
//				int rayLength = rayPattens[r][0];
//				for (int i = 1; i <= rayLength; i++)
//				{
//					Square * toSquare = &game.Squares[rayPattens[r][i]];
//					Piece * toPiece = &toSquare->Piece;
//					if (toPiece->Type != NoPiece) {
//						if (toSquare->Index == kingSquare->Index) {
//							return true;
//						}
//						break;
//					}
//				}
//			}
//			return false;
//		}
//	}
//	return true;
//}

void PerftMove(int fromSquare, int toSquare, int depth, bool promo) {
	PieceType capture = game.Squares[toSquare];
	if (capture & 7 == King)
		return;
	Move move;
	move.From = fromSquare;
	move.To = toSquare;
	move.Promotion = promo;
	//dump();
	MakeMove(move);
//	dump();
//	printf("%s", "-----\n");
	//move.Legal = true; !KingAttacked();
	side ^= 24;
	//if (move.Legal)
	Perft(depth - 1);

	UnMakeMove(move, capture);
	side ^= 24;
}

void Perft(int depth) {
	if (depth < 0)
		return;
	perftCount++;
	for (int i = 0; i < 64; i++)
	{
		PieceType pieceType = game.Squares[i];
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
						if (game.Squares[overSqr] != NoPiece)
							break;
						//en passant here
					}
					bool promo = (toSquare < 8 || toSquare > 55);
					if (game.Squares[toSquare] == NoPiece) {
						PerftMove(i, toSquare, depth, promo);
					}
				}

				int captPat = side & White ? 3 : 5;
				int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
				for (int pc = 1; pc <= pawnPatLength; pc++)
				{
					int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
					bool promo = (toSquare < 8 || toSquare > 55);
					//must be a piece of opposite color.
					if (game.Squares[toSquare] & (side ^ 24)) {
						PerftMove(i, toSquare, depth, promo);
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
					if (!(game.Squares[toSquare] & side)) {
						PerftMove(i, toSquare, depth, false);
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
					if (!(game.Squares[toSquare] & side)) {
						PerftMove(i, toSquare, depth, false);
					}
				}
				break;
			}
			default:
			{
				int raysCount = PieceTypeSquareRaysPatterns[pt - 1][i][0][0];
				for (int r = 1; r <= raysCount; r++)
				{
					int rayLength = PieceTypeSquareRaysPatterns[pt - 1][i][r][0];
					for (int s = 1; s <= rayLength; s++)
					{
						int toSquare = PieceTypeSquareRaysPatterns[pt - 1][i][r][s];
						PieceType toPiece = game.Squares[toSquare];
						if (toPiece != NoPiece) {
							if (!(toPiece & side)) {
								PerftMove(i, toSquare, depth, false);
							}
							break;
						}
						else {
							PerftMove(i, toSquare, depth, false);
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

