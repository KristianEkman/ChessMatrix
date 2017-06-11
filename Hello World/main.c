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

Piece NullPiece;

int perftCount;
void Perft(char depth);
Square * KingSquares[2];

void InitPiece(char file, char rank, enum PieceType type, enum Color color) {
	game.Squares[rank * 8 + file].Piece.Type = type;
	game.Squares[rank * 8 + file].Piece.Color = color;
	if (type == King) {
		KingSquares[color] = &game.Squares[rank * 8 + file];
	}
}

void InitGame() {
	for (char i = 0; i < 64; i++)
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

	for (char i = 0; i < 8; i++)
		InitPiece(i, 1, Pawn, White);

	InitPiece(0, 7, Rook, Black);
	InitPiece(1, 7, Knight, Black);
	InitPiece(2, 7, Bishop, Black);
	InitPiece(3, 7, Queen, Black);
	InitPiece(4, 7, King, Black);
	InitPiece(5, 7, Bishop, Black);
	InitPiece(6, 7, Knight, Black);
	InitPiece(7, 7, Rook, Black);

	for (char i = 0; i < 8; i++)
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

	for (char r = 8 - 1; r >= 0; r--)
	{
		for (char f = 0; f < 8; f++)
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
	for (char i = 0; i < 64; i++)
	{
		Square *sqr = &game.Squares[i];
		sqr->Index = i;
		sqr->BishopRays[0] = &bishopRayCount;
		sqr->BishopRays[1] = &northEastRayPatterns[i];
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

char * GetPatterns(Square * square, Piece piece) {
	switch (piece.Type)
	{	
	case Knight:
		return square->KnightsPattern;
	case King:
		return square->KingsPattern;
	default:
		return &emptyPattern;
	}
}

char ** GetRayPatterns(Square * square, enum PieceType pieceType) {
	switch (pieceType)
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

char * GetPawnPatterns(Square * square, enum Color color) {
	
	if (color == White)
		return square->WhitePawnPattern;
	else
		return square->BlackPawnPattern;
}

char * GetPawnCapturePatterns(Square * square, enum Color color) {
	if (color == White)
		return square->WhitePawnCaptures;
	else
		return square->BlackPawnCaptures;
}

bool KingAttacked() {
	Square * kingSquare = KingSquares[side];
	for (Square * fromSquare = game.Squares; fromSquare < game.Squares + 64; fromSquare++)
	{
		Piece piece = fromSquare->Piece;
		if (piece.Type != NoPiece && piece.Color != side) {
			if (piece.Type == Pawn) {
				char * pawnCapts = GetPawnCapturePatterns(fromSquare, piece.Color);
				char length = pawnCapts[0];
				for (char i = 0; i < length; i++)
				{
					if (pawnCapts[i] == kingSquare->Index) {
						return true;
					}
				}
			}

			char * pattern = GetPatterns(fromSquare, piece);
			char length = pattern[0];
			for (char p = 1; p <= length; p++)
			{
				Square * toSquare = &game.Squares[pattern[p]];
				if (pattern[p] == kingSquare->Index) {
					return true;
				}
			}

			char ** rayPattens = GetRayPatterns(fromSquare, piece.Type);
			char raysCount = rayPattens[0][0];
			for (char r = 1; r <= raysCount; r++)
			{
				char rayLength = rayPattens[r][0];
				for (char i = 1; i <= rayLength; i++)
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

void PerftMove(Square * fromSquare, Square * toSquare, char depth) {
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

void Perft(char depth) {
	if (depth < 0)
		return;
	perftCount++;
	
	for (Square * fromSquare = game.Squares; fromSquare < game.Squares + 64; fromSquare ++)
	{
		Piece piece = fromSquare->Piece;
		if (piece.Type != NoPiece && piece.Color == side) {
			if (piece.Type == Pawn) {
				char * pawnPattern = GetPawnPatterns(fromSquare, piece.Color);
				char pawnPatLength = pawnPattern[0];
				for (char pp = 1; pp <= pawnPatLength; pp++)
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

				char * pawnCaptPattern = GetPawnCapturePatterns(fromSquare, piece.Color);
				char pawnCapPatLength = pawnCaptPattern[0];
				for (char pc = 1; pc <= pawnPatLength; pc++)
				{
					Square * toSquare = &game.Squares[pawnPattern[pc]];
					if (toSquare->Piece.Type != NoPiece && toSquare->Piece.Color != side) {
						PerftMove(fromSquare, toSquare, depth);
					}
				}
			}

			char * pattern = GetPatterns(fromSquare, piece);
			char length = pattern[0];
			for (char p = 1; p <= length; p++)
			{
				Square * toSquare = &game.Squares[pattern[p]];
				if (toSquare->Piece.Type == NoPiece || toSquare->Piece.Color != side) {
					PerftMove(fromSquare, toSquare, depth);
				}
			}

			char ** rayPattens = GetRayPatterns(fromSquare, piece.Type);
			char raysCount = rayPattens[0][0];
			for (char r = 1; r <= raysCount; r++)
			{
				char rayLength = rayPattens[r][0];
				for (char i = 1; i <= rayLength; i++)
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
			//en passant
			//castling
			//promotion
		}
	}
}


int main() {
	InitBoard();
	InitGame();
	clock_t start = clock();
	Perft(5);
	clock_t stop = clock();
	float secs = (float)(stop - start) / CLOCKS_PER_SEC;
	printf("%f\n", secs);

	printf("%d\n", perftCount);
	PrintGame();
	//GetMoves();

	char str1[20];
	scanf("%s", str1);

	return 0;
}

