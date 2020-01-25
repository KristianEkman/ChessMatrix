#include "basic_structs.h"
#include "evaluation.h"
#include <stdlib.h>

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
	return game->Material[0] + game->Material[1];
}

