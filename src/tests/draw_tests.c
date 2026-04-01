#include <stdio.h>
#include <string.h>

#include "helpers.h"

#include "../main.h"
#include "../fen.h"
#include "../evaluation.h"
#include "../moves.h"

static void FillHistoryWithNonMatches(U64 repeatedHash, int historyLength)
{
	memset(g_mainGame.PositionHistory, 0, sizeof(g_mainGame.PositionHistory));
	for (int i = 0; i < historyLength; i++)
		g_mainGame.PositionHistory[i] = repeatedHash ^ (0x9e3779b97f4a7c15ULL + (U64)i * 0x100000001b3ULL);
	g_mainGame.PositionHistoryLength = historyLength;
}

TEST(IsDrawChecksOldestReachableSameSideEntry)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/8/8/6K1/8 b - - 0 1");

	U64 repeatedHash = g_mainGame.Hash;
	FillHistoryWithNonMatches(repeatedHash, 5);
	g_mainGame.PositionHistory[0] = repeatedHash;
	g_mainGame.FiftyMoveRuleCount = 5;

	Assert(IsDraw(&g_mainGame), "Repetition draw should include the oldest reachable same-side entry");
}

TEST(IsDrawScansFullReversibleWindow)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/8/8/6K1/8 b - - 0 1");

	U64 repeatedHash = g_mainGame.Hash;
	FillHistoryWithNonMatches(repeatedHash, 35);
	g_mainGame.PositionHistory[0] = repeatedHash;
	g_mainGame.FiftyMoveRuleCount = 35;

	Assert(IsDraw(&g_mainGame), "Repetition draw should scan the full reversible window");
}

TEST(IsDrawFiftyMoveBoundary)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/8/8/6K1/8 w - - 99 1");
	AssertNot(IsDraw(&g_mainGame), "99 halfmoves should not be treated as a draw");

	g_mainGame.FiftyMoveRuleCount = 100;
	Assert(IsDraw(&g_mainGame), "100 halfmoves should be treated as a draw");
}

TEST(HashIgnoresUncapturableEnPassant)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/4P3/8/8/4K3 b - - 0 1");
	U64 hashWithoutEnPassant = g_mainGame.Hash;

	ReadFen("4k3/8/8/8/4P3/8/8/4K3 b - e3 0 1");
	AssertAreEqualLongs(hashWithoutEnPassant, g_mainGame.Hash, "Uncapturable en passant should not change repetition hash identity");
}

TEST(HashKeepsCapturableEnPassant)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/3pP3/8/8/4K3 b - - 0 1");
	U64 hashWithoutEnPassant = g_mainGame.Hash;

	ReadFen("4k3/8/8/8/3pP3/8/8/4K3 b - e3 0 1");
	Assert(hashWithoutEnPassant != g_mainGame.Hash, "Capturable en passant should remain part of repetition hash identity");
}

TEST(NullMoveDoesNotPushRepetitionHistory)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/8/8/6K1/8 w - - 12 1");

	GameState prevState = g_mainGame.State;
	U64 prevHash = g_mainGame.Hash;
	int historyLength = g_mainGame.PositionHistoryLength;
	bool positionHistoryPushed = DoNullMove(&g_mainGame);

	AssertNot(positionHistoryPushed, "Null move should not push pseudo-positions into repetition history");
	AssertAreEqualInts(historyLength, g_mainGame.PositionHistoryLength, "Null move should leave repetition history length unchanged");

	UndoNullMove(prevState, &g_mainGame, prevHash, positionHistoryPushed);
	AssertAreEqualLongs(prevHash, g_mainGame.Hash, "Undoing null move should restore the original hash");
}