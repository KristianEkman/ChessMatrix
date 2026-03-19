#include <stdio.h>
#include <time.h>

#include "helpers.h"

#include "../platform.h"
#include "../utils.h"

void _runTests()
{
	// BestMoveTest();
	printf("\nPress any key to continue.");
	PlatformGetChar();
	PlatformClearScreen();
}

void runAllTests()
{
	/*DoublePawnsTest();

	if (_failedAsserts == 0)
		PrintGreen("Success! Tests are good!\n");
	printf("Press any key to continue.\n");
	int c = _getch();
	return;*/

	_failedAsserts = 0;

	RunRegisteredTests(TEST_SUITE_CORE);
	// PassedPawnTest();
	/*PositionScorePawns();
	PositionScoreKnights();
	PositionScoreCastling();*/

	clock_t start = clock();

	RunRegisteredTests(TEST_SUITE_SEARCH);

	clock_t end = clock();
	float secs = (float)(end - start) / CLOCKS_PER_SEC;
	printf("Time: %.2fs\n", secs);

	if (_failedAsserts == 0)
		PrintGreen("Success! Tests are good!\n");
	else
		PrintRed("There are failed tests.\n");

	printf("Press any key to continue.\n");
	PlatformGetChar();
	PlatformClearScreen();
}