#include "basic_structs.h"
int DepthTimeHistory[100];
int DepthTimeHistoryLength;

bool SearchDeeper(int currentDepth, int ellapsed, int myTimeLeft, int opponentTimeLeft, int score) {
	int normal = myTimeLeft / 40;
	return false;
}

void RegisterTimeDepth(int time, int depth) {
	DepthTimeHistory[depth] = time;
	DepthTimeHistoryLength = depth;
}