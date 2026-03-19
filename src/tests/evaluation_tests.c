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

TEST(UnstoppablePassedPawnEvalTest)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/1P6/8/8/8/4K3 w - - 0 1");
	Assert(GetEval(&g_mainGame) < -300, "Advanced unstoppable passer should evaluate as clearly winning in a king-and-pawn ending");
}

void MobilityRookTest()
{
	printf("%s\n", __func__);
	char *fen = "r3kbnr/ppp1pppp/2nb4/8/2P5/2N5/PP2PPPP/R1BRKB2 w Qkq - 0 1";
	ReadFen(fen);
	g_topSearchParams.MaxDepth = 1;
	Search(true);
	GetEval(&g_mainGame);
}

TEST(DoublePawnsTest)
{
	printf("%s\n", __func__);
	char *fen = "r3kbnr/pp2pppp/2np4/8/2P5/2N1P3/PP2P1PP/R1BRKB2 w Qkq - 0 1";
	ReadFen(fen);
	short score = DoublePawns(20, &g_mainGame, PAWN | WHITE);
	AssertAreEqualInts(9, score, "Double pawns score missmatch");
	AssertAreEqualInts(0, DoublePawns(8, &g_mainGame, PAWN | WHITE), "Double pawns score missmatch");
}

TEST(OpenRookFileTest)
{
	printf("%s\n", __func__);

	ReadFen("4k3/8/8/8/8/8/8/R3K3 w - - 0 1");
	AssertAreEqualInts(OPEN_ROOK_FILE, OpenRookFile(a1, &g_mainGame, ROOK | WHITE), "Open rook file score mismatch");

	ReadFen("4k3/p7/8/8/8/8/8/R3K3 w - - 0 1");
	AssertAreEqualInts(OPEN_ROOK_FILE - SEMI_OPEN_FILE, OpenRookFile(a1, &g_mainGame, ROOK | WHITE), "Semi-open rook file score mismatch");

	ReadFen("4k3/p7/8/8/8/8/P7/R3K3 w - - 0 1");
	AssertAreEqualInts(0, OpenRookFile(a1, &g_mainGame, ROOK | WHITE), "Closed rook file score mismatch");
}

TEST(KingExposureTest)
{
	printf("%s\n", __func__);
	char *protected = "rnbq1rk1/pppppppp/4bn2/8/8/3B1N2/PPPPPPPP/RNBQ1RK1 w - - 0 1";
	ReadFen(protected);
	AssertAreEqualInts(6, g_mainGame.KingSquares[0], "White king is not on square 1");
	AssertAreEqualInts(62, g_mainGame.KingSquares[1], "Black king is not on square 62");

	short whiteScore = KingExposed(1, &g_mainGame);
	short blackScore = KingExposed(62, &g_mainGame);
	AssertAreEqualInts(0, whiteScore, "Not the expected exposure score for white");
	AssertAreEqualInts(0, blackScore, "Not the expected exposure score for black");

	char *unprotected = "rnbq1rk1/ppppp3/4b3/8/8/3B4/1PPPP3/RNBQ1RK1 w - - 0 1";
	ReadFen(unprotected);
	whiteScore = KingExposed(6, &g_mainGame);
	blackScore = KingExposed(62, &g_mainGame);
	AssertAreEqualInts(48, whiteScore, "Not the expected exposure score for white");
	AssertAreEqualInts(48, blackScore, "Not the expected exposure score for black");
}

void PassedPawnTest()
{
	printf("%s\n", __func__);
	char *fen = "rnbqkbnr/3p3p/2p3p1/5p2/P7/1P5P/2PP4/RNBQKBNR w KQkq - 0 1";
	ReadFen(fen);
	short score = PassedPawn(24, &g_mainGame);
	AssertAreEqualInts(23, score, "Passed pawns score missmatch");

	score = PassedPawn(17, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(17, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(10, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(11, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(23, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(42, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(51, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(37, &g_mainGame);
	AssertAreEqualInts(23, score, "Passed pawns score missmatch");

	score = PassedPawn(46, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(55, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	ReadFen("rnbqkbnr/7p/8/5p2/1pP1P3/7P/8/RNBQKBNR w KQkq - 0 1");
	score = PassedPawn(25, &g_mainGame);
	AssertAreEqualInts(23, score, "Passed pawns score missmatch  square 25");

	score = PassedPawn(26, &g_mainGame);
	AssertAreEqualInts(23, score, "Passed pawns score missmatch  square 36");

	score = PassedPawn(28, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch  square 28");

	score = PassedPawn(37, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch square 37");
}

TEST(ProtectedByPawnTest)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/2p1p3/3b4/3N4/2P1P3/8/4K3 w - - 0 1");
	AssertAreEqualInts(16, ProtectedByPawn(d4, &g_mainGame), "White piece pawn protection mismatch");
	AssertAreEqualInts(16, ProtectedByPawn(d5, &g_mainGame), "Black piece pawn protection mismatch");
}