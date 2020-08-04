// see https://www.chessprogramming.org/Countermove_Heuristic
#include "commons.h"
#include <Windows.h>

// previous from, previous to
Move counterMoves[ENGINE_COUNT][64][64];
//const short CounterMoveBonus = 500;

void AddCounterMove(int engineId, Move move, Move previousMove) {
	counterMoves[engineId][previousMove.From][previousMove.To] = move;
}

bool IsCounterMove(int engineId, Move move, Move previousMove) {
	Move listMove = counterMoves[engineId][previousMove.From][previousMove.To];
	return (move.From == listMove.From && move.To == listMove.To);
}

void ClearCounterMoves() {
	memset(counterMoves, 0, ENGINE_COUNT * 64 * 64 * sizeof(Move));
}