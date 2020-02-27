#include <stdlib.h>
#include "commons.h"
#include "patterns.h"

void MapGameToAnnInput(Game* game, double* input) {

	for (size_t i = 0; i < 64; i++)
	{
		input[i] = AnnPieceValue[game->Squares[i]];
	}
	input[64] = game->Side == WHITE ? -1 : 1;
	
	// casling rights shouldnt affect evaluation so much as the playing side.
	input[65] = game->State & WhiteCanCastleLong ? 0.005 : 0;
	input[66] = game->State & WhiteCanCastleShort ? 0.005 : 0;
	input[67] = game->State & BlackCanCastleLong ? 0.005 : 0;
	input[68] = game->State & BlackCanCastleShort ? 0.005 : 0;
	//todo: enpassant file should be mapped here also
}