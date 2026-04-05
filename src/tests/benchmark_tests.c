#include <stdio.h>

#include "helpers.h"

#include "../benchmark.h"

TEST(BenchmarkDepthNormalizationTest)
{
	printf("%s\n", __func__);
	AssertAreEqualInts(BENCH_DEFAULT_DEPTH, NormalizeBenchDepth(0), "Benchmark depth should fall back to the default when the requested depth is not positive");
	AssertAreEqualInts(5, NormalizeBenchDepth(5), "Benchmark depth should keep a valid requested depth");
}

TEST(BenchmarkSuiteSmokeTest)
{
	printf("%s\n", __func__);
	BenchResult result = RunBenchmarkSuite(1);

	AssertAreEqualInts(1, result.Depth, "Benchmark suite should use the requested depth");
	Assert(result.PositionCount > 0, "Benchmark suite should include at least one position");
	Assert(result.TotalNodes > 0, "Benchmark suite should search at least one node");
	Assert(result.TotalTimeMs >= 0, "Benchmark suite should report a non-negative elapsed time");
}