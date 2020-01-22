#include "basic_structs.h"
#include "evaluation.h"
#include <stdlib.h>

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
			30,30,30,30,30,30,30,30,
			0, 0, 0, 0, 0, 0, 0, 0,
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0,
		   30,30,30,30,30,30,30,30,
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
			3, 3, 3,30,30, 3, 3, 3,
			6, 6, 6,20,20, 6, 6, 6,
			9, 9, 9, 9, 9, 9, 9, 9,
			12,12,12,12,12,12,12,12,
			0, 0, 0, 0, 0, 0, 0, 0,
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0,
		   12,12,12,12,12,12,12,12,
			9, 9, 9, 9, 9, 9, 9, 9,
			6, 6, 6,20,20, 6, 6, 6,
			3, 3, 3,30,30, 3, 3, 3,
			0, 0, 0, 5, 5, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
		}
	},
	//char knightPositionsValues[2][64] =
	{
		{
			-50,-40,-30,-30,-30,-30,-40,-50,
			-40,-20,  0,  5,  5,  0,-20,-40,
			-30,  5, 10, 15, 15, 10,  5,-30,
			-30,  0, 15, 20, 20, 15,  0,-30,
			-30,  5, 15, 20, 20, 15,  5,-30,
			-30,  0, 10, 15, 15, 10,  0,-30,
			-40,-20,  0,  0,  0,  0,-20,-40,
			-50,-40,-30,-30,-30,-30,-40,-50
		},{
			-50,-40,-30,-30,-30,-30,-40,-50,
			-40,-20,  0,  0,  0,  0,-20,-40,
			-30,  0, 10, 15, 15, 10,  0,-30,
			-30,  5, 15, 20, 20, 15,  5,-30,
			-30,  0, 15, 20, 20, 15,  0,-30,
			-30,  5, 10, 15, 15, 10,  5,-30,
			-40,-20,  0,  5,  5,  0,-20,-40,
			-50,-40,-30,-30,-30,-30,-40,-50
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
			-40,-30,-30,-30,-30,-30,-30,-40,
			-30,-30,  0,  0,  0,  0,-30,-30,
			-30,-10, 20, 30, 30, 20,-10,-30,
			-30,-10, 30, 40, 40, 30,-10,-30,
			-30,-10, 30, 40, 40, 30,-10,-30,
			-30,-10, 20, 30, 30, 20,-10,-30,
			-30,-20,-10,  0,  0,-10,-20,-30,
			-40,-30,-30,-20,-20,-30,-30,-40
		},{
			-40,-30,-30,-20,-20,-30,-30,-40,
			-30,-20,-10,  0,  0,-10,-20,-30,
			-30,-10, 20, 30, 30, 20,-10,-30,
			-30,-10, 30, 40, 40, 30,-10,-30,
			-30,-10, 30, 40, 40, 30,-10,-30,
			-30,-10, 20, 30, 30, 20,-10,-30,
			-30,-30,  0,  0,  0,  0,-30,-30,
			-40,-30,-30,-30,-30,-30,-30,-40
		}
	}
};

short CastlingPoints[2] = { -40, 40 };


short OpenRookFile(int square, Game* game) {
	int file = square % 8;
	short open = 30;
	for (int i = 0; i < 7; i++)
	{
		file += 8;
		if ((game->Squares[file] & PAWN)) //semi open
			open -= 15;
	}
	return open;
}

short DoublePawns(int square, Game* game, PieceType pawn) {
	int file = square % 8;
	short score = -9; //Always one pawn
	for (int i = 0; i < 7; i++)
	{
		file += 8;
		if ((game->Squares[file] == pawn))
			score += 9;
	}
	return score;
}

bool DrawByRepetition(Game* game) {
	if (game->PositionHistoryLength < 50) //This draw can happen early also
		return false;
	int start = game->PositionHistoryLength - 10; //Only checking back some moves. Possible to miss repetions but must be very rare.
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
	if (abs(game->Material[otherSide]) < 1500)
		return 0;
	short score = 0;
	// King should be on back rank.
	// Points for placement of king is handled by main position score lookup.
	if (kingColor == BLACK ) {
		if (square < 56 && square != 59 && square != 60) //back rank not center
			return 0;
		PieceType blackPawn = (BLACK | PAWN);
		if (game->Squares[square - 7] != blackPawn && square != 63)
			score += 10;
		if (game->Squares[square - 8] != blackPawn)
			score += 20;
		if (game->Squares[square - 9] != blackPawn && square != 56)
			score += 10;
	}
	else { // WHITE
		if (square > 7 && square != 3 && square != 4) //back rank not center
			return 0;
		PieceType whitePawn = (WHITE | PAWN);
		if (game->Squares[square + 7] != whitePawn && square != 0)
			score += 10;
		if (game->Squares[square + 8] != whitePawn)
			score += 20;
		if (game->Squares[square + 9] != whitePawn && square != 7)
			score += 10;
	}
	return score;
}

short GetEval(Game* game, short moveScore) {

	int score = moveScore;
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
				// todo connected rooks, no king between
			}
			break;
			//case BISHOP:
			//{
			//	//todo: bad bishops

			//	//outposts, protected by a pawn?
			//}
			// break;
			//case KNIGHT: {
			//	//outposts, protected by a pawn?
			//}
			// break;
			case PAWN: {
				score -= neg * DoublePawns(i, game, pieceType);

				// passed pawns
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
		neg += 2; // -1 --> 1 // White then black
	}
	return score;

}