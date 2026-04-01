#include <stdio.h>
#include <time.h>
#include <string.h>

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

int runAllTests(const char *testName, bool waitForInput)
{
	/*DoublePawnsTest();

	if (_failedAsserts == 0)
		PrintGreen("Success! Tests are good!\n");
	printf("Press any key to continue.\n");
	int c = _getch();
	return;*/

	bool hasFilter = testName != NULL && testName[0] != '\0';
	_failedAsserts = 0;
	clock_t start = clock();
	int testsRun = 0;

	testsRun += RunRegisteredTests(TEST_SUITE_CORE, testName);
	// PassedPawnTest();
	/*PositionScorePawns();
	PositionScoreKnights();
	PositionScoreCastling();*/

	testsRun += RunRegisteredTests(TEST_SUITE_SEARCH, testName);

	clock_t end = clock();
	float secs = (float)(end - start) / CLOCKS_PER_SEC;
	if (hasFilter && testsRun == 0)
	{
		char msg[256];
		snprintf(msg, sizeof(msg), "No test matched '%s'.\n", testName);
		PrintRed(msg);
		_failedAsserts++;
	}
	else if (hasFilter)
	{
		printf("Ran test '%s'.\n", testName);
	}

	printf("Time: %.2fs\n", secs);

	if (_failedAsserts == 0)
		PrintGreen("Success! Tests are good!\n");
	else
		PrintRed("There are failed tests.\n");

	if (waitForInput)
	{
		printf("Press any key to continue.\n");
		PlatformGetChar();
		PlatformClearScreen();
	}

	return _failedAsserts == 0 ? 0 : 1;
}