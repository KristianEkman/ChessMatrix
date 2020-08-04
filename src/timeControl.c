#include "commons.h"
#include "utils.h"
#include "search.h"
#include <stdio.h>

// engine, moveNo, depth
int DepthTimeHistory[ENGINE_COUNT][1024][MAX_DEPTH];

bool SearchDeeper(int engineId, int currentDepth, int moveNo, int ellapsed, Side side) {
	// Goal:
	//	1. Time should not end
	//	2. Time should be used

	int myTimeLeft = g_topSearchParams.BlackTimeLeft;
	int opponentTimeLeft = g_topSearchParams.WhiteTimeLeft;
	int increment = g_topSearchParams.BlackIncrement;
	if (side == WHITE) {
		opponentTimeLeft = g_topSearchParams.BlackTimeLeft;
		myTimeLeft = g_topSearchParams.WhiteTimeLeft;
		increment = g_topSearchParams.WhiteIncrement;
	}

	int normalMoves = 30; // There are probably more moves left in sudden death.
	if (g_topSearchParams.MovesTogo > 0)
		normalMoves = g_topSearchParams.MovesTogo;

	int bonus = (float)(myTimeLeft - opponentTimeLeft) * 0.75;  // Don't give whole diff as bonus.
	if (bonus < 0)
		bonus = 0; // Don't panic if behind in time.

	int givenTime = myTimeLeft / normalMoves + increment + bonus;
	int prevMaxDepth = DepthTimeHistory[engineId][moveNo - 1][0];
	int estimatedNextDepth = ellapsed * 2; // Default estimation is wild guess, but the engin loses more when this is increased.
	if (moveNo > 0 && currentDepth > 1 && prevMaxDepth > currentDepth) {
		estimatedNextDepth = DepthTimeHistory[engineId][moveNo - 1][currentDepth + 1];
	}
	// Guessing that there is time for one more depth.
	if (ellapsed + estimatedNextDepth < givenTime)
		return true;

	return false;
}

void RegisterDepthTime(int engineId, int moveNo, int depth, int time) {

	DepthTimeHistory[engineId][moveNo][depth] = time;
	DepthTimeHistory[engineId][moveNo][0] = depth;
}

void ResetDepthTimes() {
	for (int engineId = 0; engineId < ENGINE_COUNT; engineId++)
	{
		for (int i = 0; i < 1024; i++)
			DepthTimeHistory[engineId][i][0] = 0;
	}
}

void SetMoveTimeFallBack(Side side) {
	// Fallback if time control fails
	// If last depth takes much longer than estimated this sets max time when searching will end.
	int moves = 20;
	if (g_topSearchParams.MovesTogo > 0)
		moves = g_topSearchParams.MovesTogo;

	if (g_topSearchParams.TimeControl)
	{
		g_topSearchParams.MoveTime = g_topSearchParams.WhiteTimeLeft / moves;
		if (side == BLACK)
			g_topSearchParams.MoveTime = g_topSearchParams.BlackTimeLeft / moves;
	}
}