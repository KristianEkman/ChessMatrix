#include <stdlib.h>
#include "basic_structs.h"
#include "patterns.h"

void MapGameToAnnInput(Game* game, double* input) {

	double queen = MaterialMatrix[1][QUEEN];
	for (size_t i = 0; i < 64; i++)
	{
		PieceType pt = game->Squares[i] & 7;
		int side01 = (game->Squares[i] & 24) >> 4;
		input[i] = (double)MaterialMatrix[side01][pt] / (2 * queen); // normalizing between -0.5 and 0.5
	}
	input[64] = game->Side == WHITE ? -1 : 1;

	//king has no value, but its square can not be zero in the ann. It gets the value of two queens.
	input[game->KingSquares[0]] = -1;
	input[game->KingSquares[1]] = 1;

	// casling rights shouldnt affect evaluation so much as the playing side.
	input[65] = game->State & WhiteCanCastleLong ? 0.001 : 0;
	input[66] = game->State & WhiteCanCastleShort ? 0.001 : 0;
	input[67] = game->State & BlackCanCastleLong ? 0.001 : 0;
	input[68] = game->State & BlackCanCastleShort ? 0.001 : 0;
	//todo: enpassant file should be mapped here also
}