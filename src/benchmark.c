#include "benchmark.h"

#include <stdio.h>
#include <time.h>

#include "book.h"
#include "fen.h"
#include "hashTable.h"
#include "moves.h"
#include "position.h"
#include "search.h"

typedef struct
{
	const char *Name;
	const char *Fen;
} BenchPosition;

static const BenchPosition g_benchPositions[] = {
	{ "middlegame-1", "r1bqkb1r/ppp1pppp/2npn3/4P3/2P5/2N2NP1/PP1P1P1P/R1BQKB1R w KQkq - 1 1" },
	{ "middlegame-2", "r1bq2k1/p1p2pp1/2p2n1p/3pr3/7B/P1PBPQ2/5PPP/R4RK1 b - - 0 1" },
	{ "tactics", "r2q2k1/p4p1p/1rp3bB/3p4/3P1Q2/RP3P2/1KP5/4R3 w - - 3 47" },
	{ "endgame", "8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - 0 1" },
};

static long long BenchNowMs(void)
{
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static int GetBenchPositionCount(void)
{
	return (int)(sizeof(g_benchPositions) / sizeof(g_benchPositions[0]));
}

static void PrepareBenchSearch(int depth)
{
	SetSearchDefaults();
	g_topSearchParams.MaxDepth = depth;
	g_topSearchParams.TimeControl = false;
	g_topSearchParams.MoveTime = 0;
	ClearHashTable();
}

int NormalizeBenchDepth(int depth)
{
	if (depth <= 0)
		return BENCH_DEFAULT_DEPTH;
	return min(depth, MAX_DEPTH);
}

BenchResult RunBenchmarkSuite(int depth)
{
	BenchResult result = {0};
	bool savedOwnBook = OwnBook;
	int effectiveDepth = NormalizeBenchDepth(depth);
	int positionCount = GetBenchPositionCount();

	OwnBook = false;
	result.Depth = effectiveDepth;
	result.PositionCount = positionCount;

	for (int i = 0; i < positionCount; i++)
	{
		ReadFen((char *)g_benchPositions[i].Fen);
		PrepareBenchSearch(effectiveDepth);

		long long start = BenchNowMs();
		Search(false);
		result.TotalTimeMs += BenchNowMs() - start;
		result.TotalNodes += g_SearchedNodes;
	}

	StartPosition();
	OwnBook = savedOwnBook;
	return result;
}

void PrintBenchmarkSuite(int depth)
{
	bool savedOwnBook = OwnBook;
	int effectiveDepth = NormalizeBenchDepth(depth);
	int positionCount = GetBenchPositionCount();
	unsigned long long totalNodes = 0ULL;
	long long totalTimeMs = 0;

	OwnBook = false;
	printf("bench depth %d positions %d\n", effectiveDepth, positionCount);

	for (int i = 0; i < positionCount; i++)
	{
		ReadFen((char *)g_benchPositions[i].Fen);
		PrepareBenchSearch(effectiveDepth);

		long long start = BenchNowMs();
		Search(false);
		long long elapsedMs = BenchNowMs() - start;
		long long safeElapsedMs = elapsedMs > 0 ? elapsedMs : 1;
		unsigned long long nodes = g_SearchedNodes;
		unsigned long long nps = (nodes * 1000ULL) / (unsigned long long)safeElapsedMs;
		char bestMove[6] = "0000";

		if (g_topSearchParams.BestMove.From < 64 && g_topSearchParams.BestMove.To < 64)
			MoveToString(g_topSearchParams.BestMove, bestMove);

		totalNodes += nodes;
		totalTimeMs += elapsedMs;
		printf("bench %d/%d %s nodes %llu time %lld nps %llu bestmove %s\n",
			i + 1,
			positionCount,
			g_benchPositions[i].Name,
			nodes,
			elapsedMs,
			nps,
			bestMove);
	}

	{
		long long safeTotalTimeMs = totalTimeMs > 0 ? totalTimeMs : 1;
		unsigned long long totalNps = (totalNodes * 1000ULL) / (unsigned long long)safeTotalTimeMs;
		printf("bench total nodes %llu time %lld nps %llu\n", totalNodes, totalTimeMs, totalNps);
	}

	StartPosition();
	OwnBook = savedOwnBook;
}