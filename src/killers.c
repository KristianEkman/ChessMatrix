#include "commons.h"

void AddWhiteKiller(Game* game, Move move) {
	MoveCoordinates* list = game->Killers[0][game->PositionHistoryLength / 2];
	if ((list[0].From == move.From && list[0].To == move.To) ||
		(list[1].From == move.From && list[1].To == move.To))
		return; // dont add already existing killers 
	list[1] = list[0];
	list[0].From = move.From;
	list[0].To = move.To;
}

void AddBlackKiller(Game* game, Move move) {
	MoveCoordinates* list = game->Killers[1][game->PositionHistoryLength / 2];
	if ((list[0].From == move.From && list[0].To == move.To) ||
		(list[1].From == move.From && list[1].To == move.To))
		return; // dont add already existing killers 
	list[1] = list[0];
	list[0].From = move.From;
	list[0].To = move.To;
}

short killerScores[2] = { -150, 150 };

short KillerScore(Game* game, int side, Move move) {
	MoveCoordinates* list = game->Killers[side][game->PositionHistoryLength / 2];
	if (list[0].From == move.From && list[0].To == move.To)
		return killerScores[side]  * 2;
	else if (list[1].From == move.From && list[1].To == move.To)
		return killerScores[side];
	return 0;
}