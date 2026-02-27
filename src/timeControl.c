#include "commons.h"
#include "utils.h"
#include "search.h"
#include <stdio.h>
#include <stdlib.h>

int DepthTimeHistory[1024][32] = {0};
short DepthScoreHistory[1024][32] = {0};
Move DepthBestMoveHistory[1024][32] = {0};

static int ClampInt(int value, int low, int high)
{
	if (value < low)
		return low;
	if (value > high)
		return high;
	return value;
}

static bool IsSameMove(Move a, Move b)
{
	return a.From == b.From && a.To == b.To && a.MoveInfo == b.MoveInfo;
}

static void GetClockForSide(Side side, int *myTimeLeft, int *opponentTimeLeft, int *increment)
{
	*myTimeLeft = g_topSearchParams.BlackTimeLeft;
	*opponentTimeLeft = g_topSearchParams.WhiteTimeLeft;
	*increment = g_topSearchParams.BlackIncrement;
	if (side == WHITE)
	{
		*opponentTimeLeft = g_topSearchParams.BlackTimeLeft;
		*myTimeLeft = g_topSearchParams.WhiteTimeLeft;
		*increment = g_topSearchParams.WhiteIncrement;
	}
}

static void ComputeTimeBudget(Side side, int *softTargetMs, int *hardCapMs)
{
	int myTimeLeft;
	int opponentTimeLeft;
	int increment;
	GetClockForSide(side, &myTimeLeft, &opponentTimeLeft, &increment);

	myTimeLeft = max(0, myTimeLeft);
	increment = max(0, increment);

	int moveNo = g_mainGame.PositionHistoryLength;
	int estimatedMovesRemaining = 35;
	if (g_topSearchParams.MovesTogo > 0)
		estimatedMovesRemaining = g_topSearchParams.MovesTogo;
	else if (moveNo < 14)
		estimatedMovesRemaining = 45;
	else if (moveNo > 60)
		estimatedMovesRemaining = 25;

	int reserve = max(1000, (int)(myTimeLeft * 0.05f));
	int usable = max(0, myTimeLeft - reserve);
	int base = 0;
	if (estimatedMovesRemaining > 0)
		base = usable / estimatedMovesRemaining;
	base += (int)(increment * 0.7f);

	int minThink = 12;
	int soft = ClampInt(base, minThink, max(minThink, (int)(myTimeLeft * 0.10f)));

	int bonus = (int)((myTimeLeft - opponentTimeLeft) * 0.35f);
	if (bonus > 0)
		soft += bonus / max(6, estimatedMovesRemaining);

	if (myTimeLeft < 12000)
	{
		int panicSoft = (int)(myTimeLeft * 0.03f + increment * 0.8f);
		soft = min(soft, max(minThink, panicSoft));
	}

	int hardBySoft = soft * 3;
	int hardByClock = (int)(myTimeLeft * 0.25f);
	int hard = min(hardBySoft, hardByClock);
	hard = max(soft, hard);

	if (myTimeLeft < 12000)
	{
		int panicHard = (int)(myTimeLeft * 0.10f + increment);
		hard = min(hard, max(soft, panicHard));
	}

	*softTargetMs = max(minThink, soft);
	*hardCapMs = max(*softTargetMs, hard);
}

bool SearchDeeper(int currentDepth, int moveNo, int elapsedMs, Side side)
{
	int softTarget = 0;
	int hardCap = 0;
	ComputeTimeBudget(side, &softTarget, &hardCap);

	if (elapsedMs >= hardCap)
		return false;

	int estimatedNextDepth = elapsedMs * 2;
	if (moveNo > 0 && moveNo < 1024 && currentDepth > 1 && currentDepth < 31)
	{
		int prevMaxDepth = DepthTimeHistory[moveNo - 1][0];
		if (prevMaxDepth > currentDepth)
		{
			estimatedNextDepth = DepthTimeHistory[moveNo - 1][currentDepth + 1];
		}
	}

	if (elapsedMs + estimatedNextDepth < (int)(softTarget * 0.60f))
		return true;

	if (moveNo > 0 && moveNo < 1024 && currentDepth >= 3)
	{
		Move bestNow = DepthBestMoveHistory[moveNo][currentDepth];
		Move bestPrev = DepthBestMoveHistory[moveNo][currentDepth - 1];
		Move bestPrev2 = DepthBestMoveHistory[moveNo][currentDepth - 2];

		short scoreNow = DepthScoreHistory[moveNo][currentDepth];
		short scorePrev = DepthScoreHistory[moveNo][currentDepth - 1];
		short scorePrev2 = DepthScoreHistory[moveNo][currentDepth - 2];

		bool pvStable = IsSameMove(bestNow, bestPrev) && IsSameMove(bestPrev, bestPrev2);
		int swing1 = abs(scoreNow - scorePrev);
		int swing2 = abs(scorePrev - scorePrev2);
		bool scoreStable = swing1 <= 18 && swing2 <= 18;

		if (pvStable && scoreStable && elapsedMs >= (int)(softTarget * 0.60f))
			return false;

		if ((!pvStable || !scoreStable) && elapsedMs + estimatedNextDepth < hardCap)
			return true;
	}

	if (elapsedMs + estimatedNextDepth < softTarget)
		return true;

	return false;
}

void RegisterDepthTime(int moveNo, int depth, int time)
{
	if (moveNo < 0 || moveNo >= 1024 || depth < 1 || depth >= 32)
		return;
	DepthTimeHistory[moveNo][depth] = time;
	DepthTimeHistory[moveNo][0] = depth;
}

void RegisterIterationResult(int moveNo, int depth, Move bestMove, short score)
{
	if (moveNo < 0 || moveNo >= 1024 || depth < 1 || depth >= 32)
		return;

	DepthBestMoveHistory[moveNo][depth] = bestMove;
	DepthScoreHistory[moveNo][depth] = score;
}

void ResetDepthTimes()
{
	Move noMove;
	noMove.From = 0;
	noMove.To = 0;
	noMove.MoveInfo = NotAMove;
	for (int i = 0; i < 1024; i++)
	{
		DepthTimeHistory[i][0] = 0;
		for (int d = 0; d < 32; d++)
		{
			DepthScoreHistory[i][d] = 0;
			DepthBestMoveHistory[i][d] = noMove;
		}
	}
}

void SetMoveTimeFallBack(Side side)
{
	// Fallback if time control fails
	// If last depth takes much longer than estimated this sets max time when searching will end.
	if (!g_topSearchParams.TimeControl)
		return;

	int softTarget = 0;
	int hardCap = 0;
	ComputeTimeBudget(side, &softTarget, &hardCap);
	g_topSearchParams.MoveTime = hardCap;
}