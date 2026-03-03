// see https://www.chessprogramming.org/Countermove_Heuristic
#include "commons.h"
#include <string.h>

// previous from, previous to
Move counterMoves[64][64] = { 0 };
//const short CounterMoveBonus = 500;

void AddCounterMove(Move move, Move previousMove) {
	if (previousMove.From >= 64 || previousMove.To >= 64) return;
	counterMoves[previousMove.From][previousMove.To] = move;
}

bool IsCounterMove(Move move, Move previousMove) {
	if (previousMove.From >= 64 || previousMove.To >= 64) return false;
	Move listMove = counterMoves[previousMove.From][previousMove.To];
	return (move.From == listMove.From && move.To == listMove.To);
}

void ClearCounterMoves() {
	memset(counterMoves, 0, 64 * 64 * sizeof(Move));
}