#include <stdio.h>
#include "basic_structs.h"
#include "patterns.h"

Game game;
Move psuedoMoves[50];
int psuedoMoveCount = 0;

Move moves[50];
int movesCount = 0;
enum Color side = White;

Piece NullPiece;

int perftCount;

void SetPiece(int file, int rank, enum PieceType type, enum Color color) {
	game.Squares[rank * 8 + file].Piece.Type = type;
	game.Squares[rank * 8 + file].Piece.Color = color;
}

void InitGame() {
	for (int i = 0; i < 64; i++)
	{
		game.Squares[i].Piece = NullPiece;
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

void MakeMove(Move * move) {
	move->Capture = move->To->Piece;
	move->To->Piece = move->From->Piece;
	move->From->Piece = NullPiece;
	//castling, short long
	//en passant
	//promotion
}

void UnMakeMove(Move * move) {
	move->From->Piece = move->To->Piece;
	move->To->Piece = move->Capture;
}

void InitBoard() {
	for (int i = 0; i < 64; i++)
	{
		Square *sqr = &game.Squares[i];
		sqr->BishopRays[0] = &(northEastRayPatterns[i]);
		sqr->BishopRays[1] = &southEastRayPatterns[i];
		sqr->BishopRays[2] = &northWestPatterns[i];
		sqr->BishopRays[3] = &southWestPatterns[i];
		sqr->BlackPawnCaptures = &blackPawnCapturePatterns[i];
		sqr->BlackPawnPattern = &blackPawnPatterns[i];
		sqr->KingsPattern = &kingPatterns[i];
		sqr->KnightsPattern = &knightPatterns[i];
		sqr->WhitePawnCaptures = &whitePawnCapturePatterns[i];
		sqr->WhitePawnPattern = &whitePawnPatterns[i];
		sqr->QueenRays[0] = &northEastRayPatterns[i];
		sqr->QueenRays[1] = &southEastRayPatterns[i];
		sqr->QueenRays[2] = &northWestPatterns[i];
		sqr->QueenRays[3] = &southWestPatterns[i];
		sqr->QueenRays[4] = &northRayPatterns[i];
		sqr->QueenRays[5] = &eastRayPatterns[i];
		sqr->QueenRays[6] = &westPatterns[i];
		sqr->QueenRays[7] = &southRayPatterns[i];
		sqr->RookRays[0] = &northRayPatterns[i];
		sqr->RookRays[1] = &eastRayPatterns[i];
		sqr->RookRays[2] = &westPatterns[i];
		sqr->RookRays[3] = &southRayPatterns[i];
		
	}
}

int * GetPatterns(Square * square, Piece * piece) {
	switch (piece->Type)
	{
	case Pawn:
		//todo: non-capures
		return piece->Color == White ? square->WhitePawnCaptures : square->BlackPawnCaptures;
	case Knight:
		return square->KnightsPattern;
	case King:
		return square->KingsPattern;
	default:
		return &emptyPattern;
	}
}

bool KingAttacked() {

}

void Perft(int depth) {
	if (depth < 0)
		return;
	perftCount++;
	for (int s = 0; s < 64; s++)
	{
		Square * fromSquare = &game.Squares[s];
		Piece * piece = &fromSquare->Piece;
		if (piece->Color == side) {
			int * pattern = GetPatterns(fromSquare, piece);
			int length = pattern[0];
			for (int p = 1; p < length; p++)
			{
				Square * toSquare = &game.Squares[pattern[p]];
				if (toSquare->Piece.Type == NoPiece || toSquare->Piece.Color != side) {
					Move move;
					move.From = fromSquare;
					move.To = toSquare;
					MakeMove(&move);
					move.Legal = KingAttacked();
					side = !side;
					if (move.Legal)
						Perft(depth - 1);
					UnMakeMove(&move);
					side = !side;
				}
			}
			//todo:rays
		}
	}
}


int main() {
	InitBoard();
	InitGame();
	Perft(7);
	printf("%d\n", perftCount);
	PrintGame();
	//GetMoves();

	char str1[20];
	scanf("%s", str1);

	return 0;
}

