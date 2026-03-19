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

TEST(MaterialBlackPawnCapture)
{
	printf("%s\n", __func__);
	ReadFen("2r1k3/8/8/4p3/3P4/8/8/2Q1K3 w - - 0 1");
	AssertAreEqualInts(-450, TotalMaterial(&g_mainGame), "Start Material missmatch");
	AssertNot(MakePlayerMove("d4e5").Invalid, "Move was not valid");
	AssertAreEqualInts(-550, TotalMaterial(&g_mainGame), "Game Material missmatch");
}

TEST(MaterialWhiteQueenCapture)
{
	printf("%s\n", __func__);
	ReadFen("rnbqkbnr/ppp1pppp/8/3p4/4Q3/4P3/PPPP1PPP/RNB1KBNR b KQkq - 0 1");
	AssertAreEqualInts(0, TotalMaterial(&g_mainGame), "Start Material missmatch");
	AssertNot(MakePlayerMove("d5e4").Invalid, "Move was not valid");
	AssertAreEqualInts(1000, TotalMaterial(&g_mainGame), "Game Material missmatch");
}

TEST(MaterialCaptureAndPromotion)
{
	printf("%s\n", __func__);
	ReadFen("2r1k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
	AssertAreEqualInts(450, TotalMaterial(&g_mainGame), "Start Material missmatch");
	PlayerMove pm = MakePlayerMove("b7c8");
	AssertNot(pm.Invalid, "Move was not valid");
	AssertAreEqualInts(-1010, TotalMaterial(&g_mainGame), "Game Material missmatch");
	UnMakePlayerMove(pm);
	AssertAreEqualInts(450, TotalMaterial(&g_mainGame), "Start Material missmatch");
}

TEST(MaterialPromotion)
{
	printf("%s\n", __func__);
	ReadFen("2r1k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
	AssertAreEqualInts(450, TotalMaterial(&g_mainGame), "Start Material missmatch");
	AssertNot(MakePlayerMove("b7b8").Invalid, "Move was not valid");
	AssertAreEqualInts(-460, TotalMaterial(&g_mainGame), "Game Material missmatch");
}

TEST(EnPassantMaterial)
{
	printf("%s\n", __func__);
	ReadFen("r3k3/3p4/8/4P3/8/8/8/4K2R b - - 0 1");
	AssertAreEqualInts(0, TotalMaterial(&g_mainGame), "Start Material missmatch");
	AssertNot(MakePlayerMove("d7d5").Invalid, "Move was not valid");
	PlayerMove nextMove = MakePlayerMove("e5d6");
	AssertNot(nextMove.Invalid, "Move was not valid");
	AssertAreEqualInts(-100, TotalMaterial(&g_mainGame), "Game Material missmatch");
	UnMakePlayerMove(nextMove);
	AssertAreEqualInts(0, TotalMaterial(&g_mainGame), "Game Material missmatch");
}

TEST(MaterialDrawWhite)
{
	printf("%s\n", __func__);
	ReadFen("2k5/3b4/8/8/5N2/4N3/2K5/8 w - - 0 1");
	short score = GetEval(&g_mainGame);
	AssertAreEqualInts(0, score, "Game should be drawn");
}

TEST(MaterialDrawBlack)
{
	printf("%s\n", __func__);
	ReadFen("2k5/3n4/4n3/8/8/8/4B3/3K4 b - - 0 1");
	short score = GetEval(&g_mainGame);
	AssertAreEqualInts(0, score, "Game should be drawn");
}