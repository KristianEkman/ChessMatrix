#include <stdio.h>
#include "basic_structs.h"
#include "patterns.h"

Game game;
Move psuedoMoves[50];
int psuedoMoveCount = 0;

Move moves[50];
int movesCount = 0;
enum Color side = White;


void SetPiece(int file, int rank, enum PieceType type, enum Color color) {
	game.Squares[rank * 8 + file].Piece.Type = type;
	game.Squares[rank * 8 + file].Piece.Color = color;
}

void InitGame() {
	for (int i = 0; i < 64; i++)
	{
		game.Squares[i].Piece.Type = NoPiece;
		game.Squares[i].Piece.Color = NoColor;
	}

	SetPiece(0, 0, Rook, White);
	SetPiece(1, 0, Knight, White);
	SetPiece(2, 0, Bishop, White);
	SetPiece(3, 0, Queen, White);
	SetPiece(4, 0, King, White);
	SetPiece(5, 0, Bishop, White);
	SetPiece(6, 0, Knight, White);
	SetPiece(7, 0, Rook, White);

	for (int i = 0; i < 8; i++)
		SetPiece(i, 1, Pawn, White);

	SetPiece(0, 7, Rook, Black);
	SetPiece(1, 7, Knight, Black);
	SetPiece(2, 7, Bishop, Black);
	SetPiece(3, 7, Queen, Black);
	SetPiece(4, 7, King, Black);
	SetPiece(5, 7, Bishop, Black);
	SetPiece(6, 7, Knight, Black);
	SetPiece(7, 7, Rook, Black);

	for (int i = 0; i < 8; i++)
		SetPiece(i, 6, Pawn, Black);
}

char PieceChar(Piece piece) {
	switch (piece.Type)
	{
	case Pawn:
		return piece.Color == White ? 'P' : 'p';
	case Rook:
		return piece.Color == White ? 'R' : 'r';
	case Knight:
		return piece.Color == White ? 'N' : 'n';
	case Bishop:
		return piece.Color == White ? 'B' : 'b';
	case Queen:
		return piece.Color == White ? 'Q' : 'q';
	case King:
		return piece.Color == White ? 'K' : 'k';
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
			Piece piece = game.Squares[r * 8 + f].Piece;
			char c = PieceChar(piece);
			printf("| %c ", c);
		}
		printf("|\n---------------------------------\n");
	}
}

int length(int items[]) {
	return sizeof(items) / sizeof(items[0]);
}


void GetMoves() {
	for (int i = 0; i < 64; i++)
	{
		Piece * piece = &game.Squares[i].Piece;
		if (piece->Color == side) {
			//int l = length(pattern[piece->Type]);
			/*for (int j = 0; j < l; j++)
			{

			}*/
		}
	}
}

int main() {
	InitGame();
	InitPatterns();
	PrintGame();
	GetMoves();
	return 0;
}

