#include "commons.h"
#include "evaluation.h"
#include "patterns.h"
#include <stdlib.h>


//#define MOBILITY 3 // for every square a rook or bishop can go to

//white, black, (flipped, starts at A1)
//[type][side][square]
short PositionValueMatrix[7][2][64] = {
	{
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, },
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, }
	},
	// bishopPositionValues
	{
		{
			0,  0,  0,  0,  0,  0,  0, 0,
			0,  5,  0,  0,  0,  0,  5, 0,
			0,  0, 10, 10, 10, 10,  0, 0,
			0,  0, 10, 10, 10, 10,  0, 0,
			0,  0,  5, 10, 10,  5,  0, 0,
			0,  0,  5, 10, 10,  5,  0, 0,
			0,  0,  0,  0,  0,  0,  0, 0,
			0,  0,  0,  0,  0,  0,  0, 0,
		},{
			0,  0,  0,  0,  0,  0, 0, 0,
			0,  0,  0,  0,  0,  0,  0, 0,
			0,  0,  5, 10, 10,  5,  0, 0,
			0,  5,  5, 10, 10,  5,  5, 0,
			0,  0, 10, 10, 10, 10,  0, 0,
			0,  0, 10, 10, 10, 10,  0, 0,
			0,  5,  0,  0,  0,  0,  5, 0,
			0,  0,  0,  0,  0,  0,  0, 0
		}
	},
	//rookPositionValues[2][64] =
	{
		{
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			20,20,20,20,20,20,20,20,
			0, 0, 5,10,10, 5, 0, 0,
		},
		{
			0, 0, 5,10,10, 5, 0, 0,
		   20,20,20,20,20,20,20,20,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
			0, 0, 5,10,10, 5, 0, 0,
		}
	},
	// queenPositionValues[2][64] = 
	{
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
		}
	},
	//char pawnPositionValues[2][64] =
	{
		{
			0,  0,  0,  0,  0,  0,  0,  0,
		   10, 10,  0,-10,-10,  0, 10, 10,
			5,  0,  0,  5,  5,  0,  0,  5,
			0,  0, 10, 20, 20, 10,  0,  0,
			5,  5,  5, 10, 10,  5,  5,  5,
		   10, 10, 10, 20, 20, 10, 10, 10,
		   20, 20, 20, 30, 30, 20, 20, 20,
			0,  0,  0,  0,  0,  0,  0,  0
   },
		{
			0,  0,  0,  0,  0,  0,  0,  0,
		   20, 20, 20, 30, 30, 20, 20, 20,
		   10, 10, 10, 20, 20, 10, 10, 10,
			5,  5,  5, 10, 10,  5,  5,  5,
			0,  0, 10, 20, 20, 10,  0,  0,
			5,  0,  0,  5,  5,  0,  0,  5,
		   10, 10,  0,-10,-10,  0, 10, 10,
			0,  0,  0,  0,  0,  0,  0,  0,
		}
	},
	//char knightPositionsValues[2][64] =
	{
		{
			-20,-10,-10,-10,-10,-10,-10,-20,
			-10,-10,  0,  5,  5,  0,-10,-10,
			-10,  5, 10, 15, 15, 10,  5,-10,
			-10,  0, 15, 20, 20, 15,  0,-10,
			-10,  5, 15, 20, 20, 15,  5,-10,
			-10,  0, 10, 15, 15, 10,  0,-10,
			-10,-10,  0,  0,  0,  0,-10,-10,
			-20,-10,-10,-10,-10,-10,-10,-20
		},{
			-20,-10,-10,-10,-10,-10,-10,-20,
			-10,-10,  0,  5,  5,  0,-10,-10,
			-10,  5, 10, 15, 15, 10,  5,-10,
			-10,  0, 15, 20, 20, 15,  0,-10,
			-10,  5, 15, 20, 20, 15,  5,-10,
			-10,  0, 10, 15, 15, 10,  0,-10,
			-10,-10,  0,  0,  0,  0,-10,-10,
			-20,-10,-10,-10,-10,-10,-10,-20
		}
	},
	//filler for kings. they depend on age of game. i.e. end game
	{
		{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, },
		{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, }
	}
};

//[middle or end][side][square]
short KingPositionValueMatrix[2][2][64] = {
	//kingMiddleGamePositionValues[2][64] =
	{
		{
			  0,  5,  5,-10,-10,  0, 10, 5,
			-20,-20,-20,-20,-20,-20,-20,-20,
			-20,-20,-20,-20,-20,-20,-20,-20,
			-20,-30,-30,-40,-40,-30,-30,-20,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30
		},
		{
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-20,-30,-30,-40,-40,-30,-30,-20,
			-20,-20,-20,-20,-20,-20,-20,-20,
			-20,-20,-20,-20,-20,-20,-20,-20,
			  0,  5,  5,-10,-10,  0, 10,  5
		}
	},
	//kingEndGamePositionValues[2][64] = 
	{
		{
			-30,-20,-10,  0,  0,-10,-20,-30,
			-20,-10,  0,  5,  5,  0,-10,-20,
			-10,  0, 20, 30, 30, 20,  0,-10,
			  0,  5, 30, 40, 40, 30,  5,  0,
			  0,  5, 30, 40, 40, 30,  5,  0,
			-10,  0, 20, 30, 30, 20,  0,-10,
			-20,-10,  0,  5,  5,  0,-10,-20,
			-30,-20,-10,  0,  0,-10,-20,-30
		},{
			-30,-20,-10,  0,  0,-10,-20,-30,
			-20,-10,  0,  5,  5,  0,-10,-20,
			-10,  0, 20, 30, 30, 20,  0,-10,
			  0,  5, 30, 40, 40, 30,  5,  0,
			  0,  5, 30, 40, 40, 30,  5,  0,
			-10,  0, 20, 30, 30, 20,  0,-10,
			-20,-10,  0,  5,  5,  0,-10,-20,
			-30,-20,-10,  0,  0,-10,-20,-30
		}
	}
};

short CastlingPoints[2] = { -CASTLED, CASTLED };


// Array of coordinates for squares infront of the king.
// Depending on side and square of king.
int InfrontOfKingSquares[2][64][4];

// Array of coordinates for squares that could have a protecting pawn
char PawnProtectionSquares[2][64][3];

short OpenRookFile(int square, Game* game) {
	int file = square % 8;
	short open = OPEN_ROOK_FILE;
	for (int i = 0; i < 7; i++)
	{
		file += 8;
		if ((game->Squares[file] & PAWN)) //semi open
			open -= SEMI_OPEN_FILE;
	}
	return open;
}

short DoublePawns(int square, Game* game, PieceType pawn) {
	int color01 = (pawn & 24) >> 4;
	int sqBehind = square + Behind[color01];
	return (game->Squares[sqBehind] == pawn) * DOUBLE_PAWN;
}

bool IsDraw(Game* game) {
	//This draw can happen early also, but cheating for performance reasons.
	/*if (game->PositionHistoryLength < 20)
		return false;*/
	// However this optimization did not give measurable improvment

	int start = game->PositionHistoryLength - 5;
	//Only checking back some moves. Possible to miss repetions but must be quite rare.
	int end = max(0, game->PositionHistoryLength - 30);
	for (int i = start; i > end; i -= 2)
	{
		if (game->Hash == game->PositionHistory[i]) //Simplyfying to 1 fold. Should not by an disadvantage.
			return true;
	}

	return game->FiftyMoveRuleCount >= 100;

	// material draw is calculated in GetEval function
}

//When king is castled at back rank, penalty for missing pawns.
short KingExposed(int square, Game* game) {
	Side kingColor = game->Squares[square] & 24;
	int color01 = kingColor >> 4;
	PieceType pawn = PAWN | kingColor;

	short score = 0;
	for (int i = 1; i <= InfrontOfKingSquares[color01][square][0]; i++)
	{
		int protectSquare = InfrontOfKingSquares[color01][square][i];
		score += game->Squares[protectSquare] != pawn;
	}
	return score * KING_EXPOSED;
}

void CalculateInfrontOfKing() {
	for (int side = 0; side < 2; side++)
	{
		for (int square = 0; square < 64; square++)
		{
			InfrontOfKingSquares[side][square][0] = 0;
			InfrontOfKingSquares[side][square][1] = 0;
			InfrontOfKingSquares[side][square][2] = 0;
			InfrontOfKingSquares[side][square][3] = 0;
			char count = 0;
			if (side == 1) { //BLACK
				if (square < 56 || square == 59 || square == 60) //back rank not center
					InfrontOfKingSquares[side][square][0] = 0;
				else
				{
					if (square != 63)
					{
						InfrontOfKingSquares[side][square][++count] = square - 7;
						InfrontOfKingSquares[side][square][0]++;
					}
					InfrontOfKingSquares[side][square][++count] = square - 8;
					InfrontOfKingSquares[side][square][0]++;
					if (square != 56)
					{
						InfrontOfKingSquares[side][square][++count] = square - 9;
						InfrontOfKingSquares[side][square][0]++;
					}
				}
			}
			else { // WHITE
				if (square > 7 || square == 3 || square == 4) //back rank not center
					InfrontOfKingSquares[side][square][0] = 0;
				else {
					if (square != 7)
					{
						InfrontOfKingSquares[side][square][++count] = square + 9;
						InfrontOfKingSquares[side][square][0]++;
					}
					InfrontOfKingSquares[side][square][++count] = square + 8;
					InfrontOfKingSquares[side][square][0]++;
					if (square != 0)
					{
						InfrontOfKingSquares[side][square][++count] = square + 7;
						InfrontOfKingSquares[side][square][0]++;
					}
				}
			}
		}
	}
}

const int PawnPassedPoints[2][8] = { 
	{ 0, 5, 10, 20, 35, 60, 100, 200 },
	{ 200, 100, 60, 35, 20, 10, 5, 0 },
};
//No opponent pawn on files left right and infront
short PassedPawn(int square, Game* game) {
	int file = square % 8;
	int rank = square / 8;
	Side pieceColor = game->Squares[square] & 24; // (BLACK | WHITE);
	PieceType opponentPawn = PAWN | (pieceColor ^ 24);
	int freePath = 1;
	if (pieceColor == WHITE) {
		for (int i = 7 - 1; i > rank; i--) // starts at rank 7
		{
			int nextSq = (i * 8) + file;
			// to left
			if (file > 0 && game->Squares[nextSq - 1] == opponentPawn) {
				return 0;
			}
			// infront
			if (game->Squares[nextSq] == opponentPawn) {
				return 0;
			}
			else if ((game->Squares[nextSq] & 24) == BLACK) {
				freePath = 0;
			}
			//to right
			if (file < 7 && game->Squares[nextSq + 1] == opponentPawn) {
				return 0;
			}
		}
	}
	else { //black
		for (int i = 0; i < rank; i++)
		{
			int nextSq = (i * 8) + file;
			// to left
			if (file > 0 && game->Squares[nextSq - 1] == opponentPawn) {
				return 0;
			}
			// infront
			if (game->Squares[nextSq] == opponentPawn) {
				return 0;
			}
			else if ((game->Squares[nextSq] & 24) == WHITE) {
				freePath = 0;
			}
			//to right
			if (file < 7 && game->Squares[nextSq + 1] == opponentPawn) {
				return 0;
			}
		}
	}
	return PawnPassedPoints[pieceColor >> 4][rank] + (PASSED_PAWN_FREE_PATH * freePath);
}

void CalculatePawnProtection() {
	for (int side = 0; side < 2; side++)
	{
		for (int square = 0; square < 64; square++)
		{
			int file = square % 8;
			//special case for file a and h (a and h)
			if (side == 0) { //white
				if (square < 8)
					PawnProtectionSquares[side][square][0] = 0; // has no protecting pawns
				else if (file == 0)
				{
					PawnProtectionSquares[side][square][0] = 1; //one protecting pawn on file a
					PawnProtectionSquares[side][square][1] = square - 7;
				}
				else if (file == 7)
				{
					PawnProtectionSquares[side][square][0] = 1; //one protecting pawn on file h
					PawnProtectionSquares[side][square][1] = square - 9;
				}
				else {
					PawnProtectionSquares[side][square][0] = 2; //two protecting pawns on the rest
					PawnProtectionSquares[side][square][1] = square - 7;
					PawnProtectionSquares[side][square][2] = square - 9;
				}

			}
			else { // BLACK
				if (square > 56)
					PawnProtectionSquares[side][square][0] = 0; // has no protecting pawns
				else if (file == 0)
				{
					PawnProtectionSquares[side][square][0] = 1; //one protecting pawn on file a
					PawnProtectionSquares[side][square][1] = square + 9;
				}
				else if (file == 7)
				{
					PawnProtectionSquares[side][square][0] = 1; //one protecting pawn on file h
					PawnProtectionSquares[side][square][1] = square + 7;
				}
				else {
					PawnProtectionSquares[side][square][0] = 2; //two protecting pawns on the rest
					PawnProtectionSquares[side][square][1] = square + 9;
					PawnProtectionSquares[side][square][2] = square + 7;
				}
			}
		}
	}
}

void CalculatePatterns() {
	CalculatePawnProtection();
	CalculateInfrontOfKing();
}

short ProtectedByPawn(int square, Game* game) {
	Side pieceColor = game->Squares[square] & 24; // (BLACK | WHITE);
	int color01 = pieceColor >> 4;
	PieceType pawn = PAWN | pieceColor;
	int score = 0;

	for (int i = 1; i <= PawnProtectionSquares[color01][square][0]; i++)
	{
		short pps = PawnProtectionSquares[color01][square][i];
		score = (game->Squares[pps] == pawn);
	}
	return score * PAWN_PROTECT;
}

short GetEval(Game* game) {

	int score = game->Material[0] + game->Material[1];
	short posScore = 0;
	//int mobil = 0;
	int neg = -1;
	int opening = 0;
	if (game->PositionHistoryLength < 12) {
		opening = 1;
	}
	uchar pwnCount[2] = { 0, 0 };

	for (int s = 0; s < 2; s++)
	{
		short scr = 0;
		uchar bishopCount = 0;
		Piece * piece = &game->Pieces[s][0];
		while (piece != NULL)
		{
			// penalty for moving a piece more than once in the opening.
			if (opening)
			{
				if (piece->MoveCount > 1)
					scr -= SAME_TWICE;
				if (piece->Type == QUEEN)
					scr -= QUEEN_EARLY;
			}

			int i = piece->SquareIndex;
			PieceType pieceType = piece->Type;
			PieceType color = pieceType & (BLACK | WHITE);
			PieceType pt = pieceType & 7;
			posScore += PositionValueMatrix[pt][s][i];
			switch (pt)
			{
			case ROOK:
			{
				scr += OpenRookFile(i, game);
				//mobil += piece.Mobility;
			}
			break;
			case BISHOP:
			case KNIGHT: {
				bishopCount += (pt == BISHOP);
				scr += ProtectedByPawn(i, game);
			}
					   break;
			case PAWN: {
				scr -= DoublePawns(i, game, pieceType);
				scr += PassedPawn(i, game);
				scr += ProtectedByPawn(i, game);
				pwnCount[s]++;
			}
					 break;
			case KING: {
				int endGame = abs(game->Material[!game->Side01]) < ENDGAME ? 1 : 0;
				posScore += KingPositionValueMatrix[endGame][color >> 4][i];	
				scr -= KingExposed(i, game);
			}
					 break;
			default:
				break;
			}

			piece = piece->Next;
		}

		

		scr += BISHOP_PAIR * (bishopCount == 2);
		score += (neg * scr);
		neg += 2; // -1 --> 1 // White then black
	}

	// the lead is more worth when there is less material on the board
	// lead of 200 with material 2000 (two queen) -> 25p
	// supposed to make leader prefer chaning down.

	/*short materialLead = game->Material[1] - game->Material[0];
	if (materialLead)
		return score + posScore + (float)(score * 50) / (float)materialLead;*/

		//insuficient material check
	if (pwnCount[game->Side01] == 0) {
		if (game->Side == WHITE) {
			if (game->Material[0] >= -MATERIAL_N_N) {
				return max(0, score + posScore); // minimizing, 0 as best
			}
		}
		else {
			if (game->Material[1] <= MATERIAL_N_N) {
				return min(0, score + posScore); // maximizing, 0 as best. 
			}
		}
	}

	return score + posScore;
}

short TotalMaterial(Game* game) {
	return game->Material[0] + game->Material[1];
}