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
void Perft(int depth);
Square * KingSquares[2];

void InitPiece(int file, int rank, enum PieceType type, enum Color color) {
	game.Squares[rank * 8 + file].Piece.Type = type;
	game.Squares[rank * 8 + file].Piece.Color = color;
	if (type == King) {
		KingSquares[color] = &game.Squares[rank * 8 + file];
	}
}

void InitGame() {
	for (int i = 0; i < 64; i++)
	{
		game.Squares[i].Piece = NullPiece;
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
	if (move->To->Piece.Type == King)
		KingSquares[side] = move->To;

	//PrintGame();
	////GetMoves();

	//char str1[20];
	//scanf("%s", str1);

	//castling, short long
	//en passant
	//promotion
}

void UnMakeMove(Move * move) {

	if (move->From->Piece.Type == King)
		KingSquares[side] = move->From;
	move->From->Piece = move->To->Piece;
	move->To->Piece = move->Capture;

}

void InitBoard() {
	for (int i = 0; i < 64; i++)
	{
		Square *sqr = &game.Squares[i];
		sqr->Index = i;
		sqr->BishopRays[0] = &bishopRayCount;
		sqr->BishopRays[1] = &(northEastRayPatterns[i]);
		sqr->BishopRays[2] = &southEastRayPatterns[i];
		sqr->BishopRays[3] = &northWestPatterns[i];
		sqr->BishopRays[4] = &southWestPatterns[i];
		sqr->BlackPawnCaptures = &blackPawnCapturePatterns[i];
		sqr->BlackPawnPattern = &blackPawnPatterns[i];
		sqr->KingsPattern = &kingPatterns[i];
		sqr->KnightsPattern = &knightPatterns[i];
		sqr->WhitePawnCaptures = &whitePawnCapturePatterns[i];
		sqr->WhitePawnPattern = &whitePawnPatterns[i];
		sqr->QueenRays[0] = &queenRayCount;
		sqr->QueenRays[1] = &northEastRayPatterns[i];
		sqr->QueenRays[2] = &southEastRayPatterns[i];
		sqr->QueenRays[3] = &northWestPatterns[i];
		sqr->QueenRays[4] = &southWestPatterns[i];
		sqr->QueenRays[5] = &northRayPatterns[i];
		sqr->QueenRays[6] = &eastRayPatterns[i];
		sqr->QueenRays[7] = &westPatterns[i];
		sqr->QueenRays[8] = &southRayPatterns[i];
		
		sqr->RookRays[0] = &rookRayCount;
		sqr->RookRays[1] = &northRayPatterns[i];
		sqr->RookRays[2] = &eastRayPatterns[i];
		sqr->RookRays[3] = &westPatterns[i];
		sqr->RookRays[4] = &southRayPatterns[i];	
		sqr->EmptyRayPattern[0] = &emptyRayPattern;
	}
}

int * GetPatterns(Square * square, Piece * piece) {
	switch (piece->Type)
	{	
	case Knight:
		return square->KnightsPattern;
	case King:
		return square->KingsPattern;
	default:
		return &emptyPattern;
	}
}

int ** GetRayPatterns(Square * square, Piece * piece) {
	switch (piece->Type)
	{
	case Rook:
		//todo: non-capures
		return square->RookRays;
	case Bishop:
		return square->BishopRays;
	case Queen:
		return square->QueenRays;
	default:
		return square->EmptyRayPattern;
	}
}

int * GetPawnPatterns(Square * square, enum Color color) {
	
	if (color == White)
		return square->WhitePawnPattern;
	else
		return square->BlackPawnPattern;
}

int * GetPawnCapturePatterns(Square * square, enum Color color) {
	if (color == White)
		return square->WhitePawnCaptures;
	else
		return square->BlackPawnCaptures;
}

bool KingAttacked() {
	Square * kingSquare = KingSquares[side];
	for (int s = 0; s < 64; s++)
	{
		Square * fromSquare = &game.Squares[s];
		Piece * piece = &fromSquare->Piece;
		if (piece->Type != NoPiece && piece->Color != side) {

			int * pattern = GetPatterns(fromSquare, piece);
			int length = pattern[0];
			for (int p = 1; p <= length; p++)
			{
				Square * toSquare = &game.Squares[pattern[p]];
				if (pattern[p] == kingSquare->Index) {
					return true;
				}
			}

			int ** rayPattens = GetRayPatterns(fromSquare, piece);
			int raysCount = rayPattens[0][0];
			for (int r = 1; r <= raysCount; r++)
			{
				int rayLength = rayPattens[r][0];
				for (int i = 1; i <= rayLength; i++)
				{
					Square * toSquare = &game.Squares[rayPattens[r][i]];
					Piece * toPiece = &toSquare->Piece;
					if (toPiece->Type != NoPiece) {
						if (toSquare->Index == kingSquare->Index) {
							return true;
						}
						break;
					}
				}
			}
			return false;
		}
	}
	return true;
}

void PerftMove(Square * fromSquare, Square * toSquare, int depth) {
	Move move;
	move.From = fromSquare;
	move.To = toSquare;
	MakeMove(&move);
	move.Legal = !KingAttacked();
	side = !side;
	if (move.Legal)
		Perft(depth - 1);
	UnMakeMove(&move);
	side = !side;
	
}

void Perft(int depth) {
	if (depth < 0)
		return;
	perftCount++;
	
	for (int s = 0; s < 64; s++)
	{
		Square * fromSquare = &game.Squares[s];
		Piece * piece = &fromSquare->Piece;
		if (piece->Type != NoPiece && piece->Color == side) {
			if (piece->Type == Pawn) {
				int * pawnPattern = GetPawnPatterns(fromSquare, piece->Color);
				int pawnPatLength = pawnPattern[0];
				for (int pp = 1; pp <= pawnPatLength; pp++)
				{
					Square * toSquare = &game.Squares[pawnPattern[pp]];
					//tillåt inte två drag när en pjäs står ivägen
					if (pp == 2) {
						Square * overSqr = &game.Squares[pawnPattern[1]];
						if (overSqr->Piece.Type != NoPiece)
							break;
					}
					if (toSquare->Piece.Type == NoPiece) {
						PerftMove(fromSquare, toSquare, depth);
					}
				}

				int * pawnCaptPattern = GetPawnCapturePatterns(fromSquare, piece->Color);
				int pawnCapPatLength = pawnCaptPattern[0];
				for (int pc = 1; pc <= pawnPatLength; pc++)
				{
					Square * toSquare = &game.Squares[pawnPattern[pc]];
					if (toSquare->Piece.Type != NoPiece && toSquare->Piece.Color != side) {
						PerftMove(fromSquare, toSquare, depth);
					}
				}
			}

			int * pattern = GetPatterns(fromSquare, piece);
			int length = pattern[0];
			for (int p = 1; p <= length; p++)
			{
				Square * toSquare = &game.Squares[pattern[p]];
				if (toSquare->Piece.Type == NoPiece || toSquare->Piece.Color != side) {
					PerftMove(fromSquare, toSquare, depth);
				}
			}

			int ** rayPattens = GetRayPatterns(fromSquare, piece);
			int raysCount = rayPattens[0][0];
			for (int r = 1; r <= raysCount; r++)
			{
				int rayLength = rayPattens[r][0];
				for (int i = 1; i <= rayLength; i++)
				{
					Square * toSquare = &game.Squares[rayPattens[r][i]];
					Piece * toPiece = &toSquare->Piece;
					if (toPiece->Type != NoPiece) {
						if (toPiece->Color != side) {
							PerftMove(fromSquare, toSquare, depth);
						}
						break;
					} else {
						PerftMove(fromSquare, toSquare, depth);
					}
				}
			}

			//castling
			//en passant
			//promotion
		}
	}
}


int main() {
	InitBoard();
	InitGame();
	Perft(5);
	printf("%d\n", perftCount);
	PrintGame();
	//GetMoves();

	char str1[20];
	scanf("%s", str1);

	return 0;
}

