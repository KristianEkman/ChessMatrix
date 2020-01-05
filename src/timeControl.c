#include "basic_structs.h"
int DepthTimeHistory[1024][32];

bool SearchDeeper(int currentDepth, int moveNo, int ellapsed, int side) {
	int myTimeLeft = g_topSearchParams.BlackTimeLeft;
	int opponentTimeLeft = g_topSearchParams.WhiteTimeLeft;
	if (side == WHITE) {
		opponentTimeLeft = g_topSearchParams.BlackTimeLeft;
		myTimeLeft = g_topSearchParams.WhiteTimeLeft;
	}

	int normal = myTimeLeft / 30;
	int bonus = (myTimeLeft - opponentTimeLeft) / 2;  //ge inte hela differensen som bonus?

	int prevMaxDepth = DepthTimeHistory[moveNo - 1][0];
	int estimatedNextDepth = ellapsed * 5;
	if (moveNo > 0 && currentDepth > 1 && prevMaxDepth > currentDepth) {
		estimatedNextDepth = DepthTimeHistory[moveNo - 1][currentDepth + 1];
	}
	
	//Uppskattar om man hinner med ett djup till utan att komma efter motståndaren
	if (ellapsed + estimatedNextDepth < normal + bonus)
		return true;

	return false; 
}

void RegisterDepthTime(int moveNo, int depth, int time) {
	DepthTimeHistory[moveNo][depth] = time;
	DepthTimeHistory[moveNo][0] = depth;
}

void ResetDepthTimes() {
	for (int i = 0; i < 1024; i++)
		DepthTimeHistory[i][0] = 0;
}