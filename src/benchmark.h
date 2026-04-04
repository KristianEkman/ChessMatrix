#pragma once

#include "commons.h"

#define BENCH_DEFAULT_DEPTH 7

typedef struct
{
	int Depth;
	int PositionCount;
	unsigned long long TotalNodes;
	long long TotalTimeMs;
} BenchResult;

int NormalizeBenchDepth(int depth);
BenchResult RunBenchmarkSuite(int depth);
void PrintBenchmarkSuite(int depth);