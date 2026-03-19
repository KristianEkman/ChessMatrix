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

#ifdef _DEBUG

TEST(PerftSaveHashTest)
{
	printf("%s\n", __func__);
	perftSaveHashCount = 0;
	collisionCount = 0;
	ReadFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
	PerftSaveHash(3);
	printf("\nHash entries: %d\n", perftSaveHashCount);
	printf("\nCollisions: %d\n", collisionCount);
}

TEST(EnpassantCollisionsTest)
{
	printf("%s\n", __func__);
	perftSaveHashCount = 0;
	collisionCount = 0;
	ReadFen("rnbqkbnr/ppp2pp1/3p4/1P2p3/7p/3PP3/P1P2PPP/RNBQKBNR w KQkq - 0 1");
	PerftSaveHash(3);
	printf("\nHash entries: %d\n", perftSaveHashCount);
	printf("\nCollisions: %d\n", collisionCount);
}

#endif

TEST(PerftHashDbTest)
{
	printf("%s\n", __func__);
	ClearHashTable();
	ReadFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
	PerftHashDb(4);
	PrintHashStats();
}

TEST(PerfTestPosition2)
{
	printf("%s\n", __func__);
	char fen[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0";
	PerftTest(fen, 4);
	AssertAreEqualInts(757163, perftResult.Captures, "Captures missmatch");
	AssertAreEqualInts(128013, perftResult.Castles, "Castles missmatch");
	AssertAreEqualInts(1929, perftResult.Enpassants, "En passants missmatch");
	AssertAreEqualInts(15172, perftResult.Promotions, "Promotion missmatch");
}

TEST(PerftTestStart)
{
	printf("%s\n", __func__);
	char startFen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0";
	int count = PerftTest(startFen, 5);
	AssertAreEqualInts(4865609, count, "Perft Count missmatch");
}