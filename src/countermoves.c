// see https://www.chessprogramming.org/Countermove_Heuristic
#include "commons.h"
#include <Windows.h>

// previous from, previous to
Move counterMoves[64][64];
//const short CounterMoveBonus = 500;

void AddCounterMove(Move move, Move previousMove) {
	counterMoves[previousMove.From][previousMove.To] = move;
}

bool IsCounterMove(Move move, Move previousMove) {
	Move listMove = counterMoves[previousMove.From][previousMove.To];
	return (move.From == listMove.From && move.To == listMove.To);
}

void ClearCounterMoves() {
	memset(counterMoves, 0, 64 * 64 * sizeof(Move));
}