#include "commons.h"
#include "evaluation.h"
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

bool IsDraw(Game* game) {
	//This draw can happen early also, but cheating for performance reasons.
	/*if (game->PositionHistoryLength < 20) 
		return false;*/
	// However this optimization did not give measurable improvment

	int start = game->PositionHistoryLength - 20; //Only checking back some moves. Possible to miss repetions but must be quite rare.
	int end = game->PositionHistoryLength - (int)2;
	for (size_t i = start; i < end; i++)
	{
		if (game->Hash == game->PositionHistory[i]) //Simplyfying to 1 fold. Should not by an disadvantage.
			return true;
	}

	//Draw by material if both players have two knights or less material.
	// todo: And no pawn.
	//if (game->Material[0] >= -MATERIAL_N_N && game->Material[1] <= MATERIAL_N_N)
	//	return true;

	return game->FiftyMoveRuleCount >= 100;
}

// Move score is much faster than evaluation. It is used during move generation to get good sortting of moves.
short GetMoveScore(Game* game) {
	return game->Material[0] + game->Material[1] + game->PositionScore;
}

short KingExposed(int square, Game* game) {
	// If opponent has alot of material, the king should be protected by pawns.
	// More points for square infront
	// less for diagonals.

	PieceType kingColor = game->Squares[square] & (BLACK | WHITE);
	int otherSide = (kingColor >> 4) ^ 1;
	if (abs(game->Material[otherSide]) < KING_NEEDS_PROTECTION)
		return 0;

	short score = 0;
	//short score = game->Squares[InfrontOfKing[game->Side01][square]];

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

//No opponent pawn on files left right and infront
short PassedPawn(int square, Game* game) {
	int file = square % 8;
	int rank = square / 8;
	Side pieceColor = game->Squares[square] & 24; // (BLACK | WHITE);
	PieceType opponentPawn = PAWN | (pieceColor ^ 24);
	int steps = 0;
	if (pieceColor == WHITE) {
		for (int i = 7 - 1; i > rank; i--) // starts at rank 7
		{
			// to left
			if (file > 0 && game->Squares[(i * 8) + file - 1] == opponentPawn) {
				return 0;
			}
			// infront
			if (game->Squares[(i * 8) + file] == opponentPawn) {
				return 0;
			}
			//toright
			if (file < 7 && game->Squares[(i * 8) + file + 1] == opponentPawn) {
				return 0;
			}
		}
		steps = rank - 1;
	}
	else { //black
		for (int i = 0; i < rank; i++)
		{
			// to left
			if (file > 0 && game->Squares[(i * 8) + file - 1] == opponentPawn) {
				return 0;
			}
			// infront
			if (game->Squares[(i * 8) + file] == opponentPawn) {
				return 0;
			}
			//toright
			if (file < 7 && game->Squares[(i * 8) + file + 1] == opponentPawn) {
				return 0;
			}
		}
		steps = 6 - rank;
	}
	return PASSED_PAWN * steps;
}

short ProtectedByPawn(int square, Game* game) {
	Side pieceColor = game->Squares[square] & 24; // (BLACK | WHITE);
	PieceType pawn = PAWN | pieceColor;
	int file = square % 8;

	if (pieceColor == WHITE) {
		//special case for file a and h (a and h)
		if (square < 8)
			return 0;
		if (file == 0 && game->Squares[square - 7] == pawn)
			return PAWN_PROTECT;
		if (file == 7 && game->Squares[square - 9] == pawn)
			return PAWN_PROTECT;
		if (game->Squares[square - 7] == pawn || game->Squares[square - 9] == pawn)
			return PAWN_PROTECT;
	}
	else { // BLACK
		if (square > 56)
			return 0;
		if (file == 0 && game->Squares[square + 9] == pawn)
			return PAWN_PROTECT;
		if (file == 7 && game->Squares[square + 7] == pawn)
			return PAWN_PROTECT;
		if (game->Squares[square + 7] == pawn || game->Squares[square + 9] == pawn)
			return PAWN_PROTECT;
	}
	return 0;
}

short GetEval(Game* game, short moveScore) {

	int score = moveScore;
	//int mobil = 0;
	int neg = -1;
	int opening = 0;
	if (game->PositionHistoryLength < 12) {
		opening = 1;
	}
	for (size_t s = 0; s < 2; s++)
	{
		for (size_t p = 0; p < 16; p++)
		{
			Piece piece = game->Pieces[s][p];
			if (piece.Off)
				continue;

			// penalty for moving a piece more than once in the opening.
			if (opening)
			{
				if (piece.MoveCount > 1)
					score += (neg * SAME_TWICE);
				if (piece.Type == QUEEN)
					score += (neg * QUEEN_EARLY);
			}

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
			case KNIGHT: {
				score += neg * ProtectedByPawn(i, game);
			}
			 break;
			case PAWN: {
				score -= neg * DoublePawns(i, game, pieceType);
				score += neg * PassedPawn(i, game);
				score += neg * ProtectedByPawn(i, game);
			}
					 break;
			case KING: {
				score -= neg * KingExposed(i, game);
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