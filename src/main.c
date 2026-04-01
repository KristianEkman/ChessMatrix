#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "commons.h"
#include "platform.h"
#include "errorHandling.h"
#include "fen.h"
#include "position.h"
#include "patterns.h"
#include "main.h"
#include "utils.h"
#include "tests/tests.h"
#include "evaluation.h"
#include "sort.h"
#include "hashTable.h"
#include "timeControl.h"
#include "moves.h"
#include "search.h"
#include "book.h"
#include "version.h"
#include "uci.h"
#include "interactive.h"

Game g_mainGame = {0};

static int HandleCommandLineMode(int argc, char *argv[])
{
	if (argc >= 2 && strcmp(argv[1], "test") == 0)
	{
		const char *testName = argc >= 3 ? argv[2] : NULL;
		return runAllTests(testName, false);
	}

	return -1;
}

int main(int argc, char *argv[])
{
	const char *disableSignalHandlers = getenv("CM_DISABLE_SIGNAL_HANDLERS");
	if (disableSignalHandlers == NULL || disableSignalHandlers[0] == '\0' || disableSignalHandlers[0] == '0')
	{
		InstallUnhandledErrorHandlers();
	}
	SwitchSignOfWhitePositionValue();
	SetSearchDefaults();
	ResetDepthTimes();
	GenerateZobritsKeys();
	AllocateHashTable(1024);
	ClearHashTable();
	InitLmr();
	CalculatePatterns();
	StartPosition();
	OwnBook = false;
#ifndef _DEBUG // loadbook is to slow in debug mode.
			   // LoadBook("openings.abk");
#endif		   // DEBUG

	printf("info Built date %s\n", BuildDate);
	printf("info Branch %s\n", GitBranch);
	printf("info Commit %s\n", GitCommit);
	printf("Initialized\n");

	int commandLineStatus = HandleCommandLineMode(argc, argv);
	if (commandLineStatus != -1)
	{
		MarkQuitRequested();
		return commandLineStatus;
	}

	EnterUciMode();
	return 0;
}
