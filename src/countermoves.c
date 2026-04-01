// see https://www.chessprogramming.org/Countermove_Heuristic
#include "commons.h"
#include <string.h>

// previous from, previous to
Move counterMoves[64][64] = { 0 };
//const short CounterMoveBonus = 500;

static bool HasCounterMoveIndex(Move move) {
	return move.From < 64 && move.To < 64;
}

void AddCounterMove(Move move, Move previousMove) {
	if (!HasCounterMoveIndex(move) || !HasCounterMoveIndex(previousMove))
		return;

	counterMoves[previousMove.From][previousMove.To] = move;
}

bool IsCounterMove(Move move, Move previousMove) {
	if (!HasCounterMoveIndex(move) || !HasCounterMoveIndex(previousMove))
		return false;

	Move listMove = counterMoves[previousMove.From][previousMove.To];
	return (move.From == listMove.From && move.To == listMove.To);
}

void ClearCounterMoves() {
	memset(counterMoves, 0xFF, sizeof(counterMoves));
}