#include <stdio.h>
#include <time.h>
#include <string.h>

#include "helpers.h"

#include "../main.h"
#include "../fen.h"
#include "../commons.h"
#include "../utils.h"
#include "../hashTable.h"
#include "../evaluation.h"
#include "../search.h"
#include "../moves.h"
#include "../platform.h"
#include "../bitboards.h"

TEST(HashKeyTest)
{
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
	U64 hash1 = g_mainGame.Hash;
	MakePlayerMove("g1f3");
	MakePlayerMove("g8f6");
	MakePlayerMove("f3g1");
	MakePlayerMove("f6g8");
	U64 hash2 = g_mainGame.Hash;

	AssertAreEqualLongs(hash1, hash2, "Hash keys should be equal");
}

TEST(HashTableRoundTrip)
{
	printf("%s\n", __func__);
	ClearHashTable();
	U64 hash = 0x1234567890ABCDEF;
	short expected = 3000;
	Move move1 = ParseMove("b1c1", PlainMove);
	AddHashScore(hash, expected, 1, EXACT, move1);
	short score = 0;
	Move move;
	GetScoreFromHash(hash, 1, &score, &move, 3000, 0);
	AssertAreEqualInts(expected, score, "hash table score missmatch");

	U64 hash2 = hash + 1;
	short expected2 = -4000;
	Move move2 = ParseMove("c2e2", PlainMove);
	AddHashScore(hash2, expected2, 1, EXACT, move2);

	short score2;
	GetScoreFromHash(hash2, 1, &score2, &move, 0, 0);
	AssertAreEqualInts(expected2, score2, "hash table score missmatch");

	GetScoreFromHash(hash, 1, &score, &move, 0, 0);
	AssertAreEqualInts(expected, score, "hash table score missmatch");
}

TEST(HashTableDepthTest)
{
	printf("%s\n", __func__);
	ClearHashTable();
	U64 hash = 0x1234567890ABCDEF;

	Move move1 = ParseMove("a6c7", PlainMove);
	AddHashScore(hash, 3000, 2, EXACT, move1);
	int depth = 2;
	short score;
	Move move;
	GetScoreFromHash(hash, depth, &score, &move, 100, 200);
	AssertAreEqualInts(3000, score, "hash table score missmatch");

	Move move2 = ParseMove("c2c2", PlainMove);
	AddHashScore(hash, 4000, 1, EXACT, move2);
	short score2 = 0;
	GetScoreFromHash(hash, 2, &score2, &move, 30, 60);
	AssertAreEqualInts(3000, score2, "smaller depth should not replace score");

	Move move3 = ParseMove("e3g4", PlainMove);
	AddHashScore(hash, 5000, 3, EXACT, move3);
	GetScoreFromHash(hash, 3, &score, &move, 30, 31);
	AssertAreEqualInts(5000, score, "larger depth should replace value");
}

static void HashTablePerformance(int iterations)
{
	printf("%s\n", __func__);
	ClearHashTable();
	U64 hash = Llrand();
	short expected = MIN_SCORE;
	int depth = 1;
	short score = 0;
	Move move;

	for (int i = 0; i < iterations; i++)
	{
		expected++;
		if (expected > MAX_SCORE)
			expected = MIN_SCORE;
		hash++;
		Move move1 = ParseMove("b1b1", PlainMove);
		AddHashScore(hash, expected, 1, EXACT, move1);
		Assert(GetScoreFromHash(hash, depth, &score, &move, 100, 200), "No score returned from hash");
		AssertAreEqualInts(expected, score, "hash table score missmatch");
	}
}

TEST(HashTablePerformanceTest)
{
	TimedTest(50000000, HashTablePerformance);
}

TEST(MoveHashMatchesRebuiltFen)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1");
	PlayerMove pm = MakePlayerMove("e2e4");
	AssertNot(pm.Invalid, "Move was not valid");

	U64 hashAfterMove = g_mainGame.Hash;
	char fen[100];
	WriteFen(fen);
	ReadFen(fen);

	AssertAreEqualLongs(hashAfterMove, g_mainGame.Hash, "Move hash should match the hash rebuilt from FEN");
}

TEST(PromotionHashMatchesRebuiltFen)
{
	printf("%s\n", __func__);
	ReadFen("4k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
	PlayerMove pm = MakePlayerMove("b7b8q");
	AssertNot(pm.Invalid, "Move was not valid");

	U64 hashAfterMove = g_mainGame.Hash;
	char fen[100];
	WriteFen(fen);
	ReadFen(fen);

	AssertAreEqualLongs(hashAfterMove, g_mainGame.Hash, "Promotion hash should match the hash rebuilt from FEN");
}