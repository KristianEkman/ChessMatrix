#include "commons.h"
#include "evaluation.h"
#include <stdlib.h>

#define CASTLED 40
#define OPEN_ROOK_FILE 19
#define SEMI_OPEN_FILE 15
#define DOUBLE_PAWN 9
#define KING_NEEDS_PROTECTION 1500
#define KING_EXPOSED_INFRONT 22
#define KING_EXPOSED_DIAGONAL 12
#define PASSED_PAWN 23
#define MOBILITY 3 // for every square a rook or bishop can go to

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
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			20,20,20,20,20,20,20,20,
			0, 0, 0, 0, 0, 0, 0, 0,
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0,
		   20,20,20,20,20,20,20,20,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
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
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 5, 5, 0, 0, 0,
			3, 3, 3,28,28, 3, 3, 3,
			6, 6, 6,10,10, 6, 6, 6,
			9, 9, 9, 9, 9, 9, 9, 9,
			12,12,12,12,12,12,12,12,
			0, 0, 0, 0, 0, 0, 0, 0,
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0,
		   12,12,12,12,12,12,12,12,
			9, 9, 9, 9, 9, 9, 9, 9,
			6, 6, 6,10,10, 6, 6, 6,
			3, 3, 3,28,28, 3, 3, 3,
			0, 0, 0, 5, 5, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
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
			 20, 30, 10,  0,  0, 10, 30, 20,
			 20, 20,  0,  0,  0,  0, 20, 20,
			-10,-20,-20,-20,-20,-20,-20,-10,
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
			-10,-20,-20,-20,-20,-20,-20,-10,
			 20, 20,  0,  0,  0,  0, 20, 20,
			 20, 30, 10,  0,  0, 10, 30, 20
		}
	},
	//kingEndGamePositionValues[2][64] = 
	{
		{
			-20,-10,-10,-10,-10,-10,-10,-20,
			-10,-10,  0,  0,  0,  0,-10,-10,
			-10,  0, 20, 30, 30, 20,  0,-10,
			-10,  5, 30, 40, 40, 30,  5,-10,
			-10,  5, 30, 40, 40, 30,  5,-10,
			-10,  0, 20, 30, 30, 20,  0,-10,
			-10,-10,  0,  5,  5,  0,-10,-10,
			-20,-10,-10,-10,-10,-10,-10,-20
		},{
			-20,-10,-10,-10,-10,-10,-10,-20,
			-10,-10,  0,  0,  0,  0,-10,-10,
			-10,  0, 20, 30, 30, 20,  0,-10,
			-10,  5, 30, 40, 40, 30,  5,-10,
			-10,  5, 30, 40, 40, 30,  5,-10,
			-10,  0, 20, 30, 30, 20,  0,-10,
			-10,-10,  0,  5,  5,  0,-10,-10,
			-20,-10,-10,-10,-10,-10,-10,-20
		}
	}
};

short CastlingPoints[2] = { -CASTLED, CASTLED };

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
	int file = square % 8;
	short score = -DOUBLE_PAWN; //Always one pawn
	for (int i = 0; i < 7; i++)
	{
		file += 8;
		if ((game->Squares[file] == pawn))
			score += DOUBLE_PAWN;
	}
	return score;
}

bool DrawByRepetition(Game* game) {
	if (game->PositionHistoryLength < 50) //This draw can happen early also
		return false;
	int start = game->PositionHistoryLength - 10; //Only checking back some moves. Possible to miss repetions but must be quite rare.
	int end = game->PositionHistoryLength - (int)2;
	for (size_t i = start; i < end; i++)
	{
		if (game->Hash == game->PositionHistory[i]) //Simplyfying to 1 fold. Should not by an disadvantage.
			return true;
	}
	return false;

	// todo 50 move rule.
}

short GetMoveScore(Game* game) {
	return game->Material[0] + game->Material[1] + game->PositionScore;
}

short KingExposed(int square, Game* game) {
	// If opponent has atleast 1500 material, the king should be protected by pawns.
	// 10p for square infront
	// 5p for diagonals.

	PieceType kingColor = game->Squares[square] & (BLACK | WHITE);
	int otherSide = (kingColor >> 4) ^ 1;
	if (abs(game->Material[otherSide]) < KING_NEEDS_PROTECTION)
		return 0;
	short score = 0;
	// King should be on back rank.
	// Points for placement of king is handled by main position score lookup.
	if (kingColor == BLACK) {
		if (square < 56 && square != 59 && square != 60) //back rank not center
			return 0;
		PieceType blackPawn = (BLACK | PAWN);
		if (game->Squares[square - 7] != blackPawn && square != 63)
			score += KING_EXPOSED_DIAGONAL;
		if (game->Squares[square - 8] != blackPawn)
			score += KING_EXPOSED_INFRONT;
		if (game->Squares[square - 9] != blackPawn && square != 56)
			score += KING_EXPOSED_DIAGONAL;
	}
	else { // WHITE
		if (square > 7 && square != 3 && square != 4) //back rank not center
			return 0;
		PieceType whitePawn = (WHITE | PAWN);
		if (game->Squares[square + 7] != whitePawn && square != 0)
			score += KING_EXPOSED_DIAGONAL;
		if (game->Squares[square + 8] != whitePawn)
			score += KING_EXPOSED_INFRONT;
		if (game->Squares[square + 9] != whitePawn && square != 7)
			score += KING_EXPOSED_DIAGONAL;
	}
	return score;
}

short PassedPawn(int square, Game* game) {
	//No opponent pawn on files left right and infron
	int file = square % 8;
	int rank = square / 8;
	Side pieceColor = game->Squares[square] & 24; // (BLACK | WHITE);
	PieceType opponentPawn = PAWN | (pieceColor ^ 24);
	if (pieceColor == WHITE) {
		for (int i = 7 - 1; i > rank; i--) // starts at rank 7
		{
			// to left
			if (file > 0 && game->Squares[(i * 8) + file - 1] == opponentPawn) {
				return 0;
			}
			// infron
			if (game->Squares[(i * 8) + file] == opponentPawn) {
				return 0;
			}
			//toright
			if (file < 7 && game->Squares[(i * 8) + file + 1] == opponentPawn) {
				return 0;
			}
		}
	}
	else { //black
		for (int i = 0; i < rank; i++)
		{
			// to left
			if (file > 0 && game->Squares[(i * 8) + file - 1] == opponentPawn) {
				return 0;
			}
			// infron
			if (game->Squares[(i * 8) + file] == opponentPawn) {
				return 0;
			}
			//toright
			if (file < 7 && game->Squares[(i * 8) + file + 1] == opponentPawn) {
				return 0;
			}
		}
	}
	return PASSED_PAWN;
}

short GetEval(Game* game, short moveScore) {

	int score = moveScore;
	//int mobil = 0;
	int neg = -1;
	for (size_t s = 0; s < 2; s++)
	{
		for (size_t p = 0; p < 16; p++)
		{
			Piece piece = game->Pieces[s][p];
			if (piece.Off)
				continue;
			int i = piece.SquareIndex;
			PieceType pieceType = piece.Type;
			PieceType color = pieceType & (BLACK | WHITE);
			PieceType pt = pieceType & 7;

			switch (pt)
			{
			case ROOK:
			{
				score += neg * OpenRookFile(i, game);
				//mobil += piece.Mobility;
			}
			break;
			case BISHOP:
			{
				//mobil += piece.Mobility;
			}
			 break;
			//case KNIGHT: {
			//	//outposts, protected by a pawn?
			// Mobility of knights is set by piecetypeposition matrix
			//}
			// break;
			case PAWN: {
				score -= neg * DoublePawns(i, game, pieceType);
				score += neg * PassedPawn(i, game);
			}
					 break;
			case KING: {
				score -= neg * KingExposed(i, game, color);
			}
					 break;
			default:
				break;
			}
		}
		//score += neg * mobil * MOBILITY;
		neg += 2; // -1 --> 1 // White then black
	}
	return score;

}

short TotalMaterial(Game* game) {
	return game->Material[0] + game->Material[1];
}