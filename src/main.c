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
#include "benchmark.h"
#include "ann_training.h"
#include "evaluation.h"
#include "hashTable.h"
#include "timeControl.h"
#include "moves.h"
#include "search.h"
#include "book.h"
#include "version.h"
#include "uci.h"
#include "interactive.h"

Game g_mainGame = {0};

static int ParseCliInt(const char *text, int *parsed)
{
	char *end = NULL;
	long value;

	if (text == NULL || parsed == NULL)
		return -1;

	value = strtol(text, &end, 10);
	if (end == text || *end != '\0')
		return -1;

	*parsed = (int)value;
	return 0;
}

static int ConfigureBenchAnnEval(int argc, char *argv[], int argStartIndex)
{
	for (int index = argStartIndex; index < argc; index++)
	{
		char *arg = argv[index];
		if (strcmp(arg, "--ann-enable") == 0)
		{
			SetAnnEvalEnabled(true);
			continue;
		}

		if (index + 1 >= argc)
		{
			fprintf(stderr, "Missing value after %s.\n", arg);
			return 1;
		}

		if (strcmp(arg, "--ann-eval") == 0)
		{
			if (LoadAnnEvalWeights(argv[++index]) != 0)
			{
				fprintf(stderr, "Failed to load ANN eval weights from %s.\n", argv[index]);
				return 1;
			}
			SetAnnEvalEnabled(true);
		}
		else if (strcmp(arg, "--ann-blend") == 0)
		{
			int blendPercent = 0;
			if (ParseCliInt(argv[++index], &blendPercent) != 0)
				return 1;
			SetAnnEvalBlendPercent(blendPercent);
		}
		else if (strcmp(arg, "--ann-max-correction") == 0)
		{
			int maxCorrection = 0;
			if (ParseCliInt(argv[++index], &maxCorrection) != 0)
				return 1;
			SetAnnEvalMaxCorrectionCp(maxCorrection);
		}
		else if (strcmp(arg, "--ann-min-phase") == 0)
		{
			int minPhase = 0;
			if (ParseCliInt(argv[++index], &minPhase) != 0)
				return 1;
			SetAnnEvalMinPhase(minPhase);
		}
		else if (strcmp(arg, "--ann-max-base-eval") == 0)
		{
			int maxBaseEval = 0;
			if (ParseCliInt(argv[++index], &maxBaseEval) != 0)
				return 1;
			SetAnnEvalMaxBaseEvalCp(maxBaseEval);
		}
		else
		{
			fprintf(stderr, "Unknown bench option: %s\n", arg);
			return 1;
		}
	}

	return 0;
}

static int HandleCommandLineMode(int argc, char *argv[])
{
	if (argc >= 2 && strcmp(argv[1], "test") == 0)
	{
		const char *testName = argc >= 3 ? argv[2] : NULL;
		return runAllTests(testName, false);
	}

	if (argc >= 2 && strcmp(argv[1], "bench") == 0)
	{
		int depth = BENCH_DEFAULT_DEPTH;
		int argStartIndex = 2;
		ResetAnnEvalState();
		if (argc >= 3 && argv[2][0] != '-')
		{
			depth = atoi(argv[2]);
			argStartIndex = 3;
		}
		if (ConfigureBenchAnnEval(argc, argv, argStartIndex) != 0)
			return 1;
		PrintBenchmarkSuite(depth);
		return 0;
	}

	if (argc >= 2 && strcmp(argv[1], "train-ann") == 0)
	{
		return RunAnnTrainingFromArgs(argc, argv, 2);
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
	ResetAnnEvalState();
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
