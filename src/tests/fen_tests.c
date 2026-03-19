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

TEST(FenTest)
{
	char fen1[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 1";
	ReadFen(fen1);
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen1, outFen, "Start and end fen differ");
}

TEST(FenTestHalfMoves)
{
	char fen1[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 23";
	ReadFen(fen1);
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen1, outFen, "Start and end fen differ");
}

TEST(FenEnppasantTest)
{
	printf("%s\n", __func__);
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	MakePlayerMove("d2d4");
	char fen[100];
	WriteFen(fen);
	AssertAreEqual(fen, "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0", "Fen missmatch");

	PlayerMove pm2 = MakePlayerMove("d7d6");
	UnMakePlayerMove(pm2);
	WriteFen(fen);
	AssertAreEqual(fen, "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0", "Fen missmatch");
}

TEST(FenWithPromotedQueenKeepsMaterial)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/8/8/QQ6/4K3 w - - 0 1");
	AssertAreEqualInts(-2000, TotalMaterial(&g_mainGame), "FEN with an extra promoted queen should keep both queens in material");
	Assert(GetEval(&g_mainGame) < -1500, "FEN with two white queens should evaluate as a decisive white advantage");
}

TEST(FenWithPromotedRookKeepsPieceList)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/8/8/RR6/4K3 w - - 0 1");

	int rookCount = 0;
	for (Piece *piece = &g_mainGame.Pieces[0][0]; piece != NULL; piece = piece->Next)
	{
		if ((piece->Type & 7) == ROOK)
			rookCount++;
	}

	AssertAreEqualInts(2, rookCount, "FEN with an extra promoted rook should keep both rooks in the piece list");
	AssertAreEqualInts(-1100, TotalMaterial(&g_mainGame), "FEN with two white rooks should keep both rooks in material");
}

TEST(FenReadResetsSearchState)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1");
	AssertNot(MakePlayerMove("e2e4").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("e8d8").Invalid, "Move was not valid");
	Assert(g_mainGame.PositionHistoryLength > 0, "History should contain played moves before the reset test");
	Assert(g_mainGame.FiftyMoveRuleCount > 0, "Halfmove clock should advance before the reset test");

	ReadFen("4k3/8/8/8/8/8/QQ6/4K3 w - -");
	AssertAreEqualInts(0, g_mainGame.PositionHistoryLength, "ReadFen should clear position history length");
	AssertAreEqualInts(0, g_mainGame.FiftyMoveRuleCount, "ReadFen should reset the halfmove clock when the FEN omits it");
	AssertNot(IsDraw(&g_mainGame), "Freshly loaded FEN should not inherit repetition draw state");
	Assert(GetEval(&g_mainGame) < -1500, "Freshly loaded promoted-piece FEN should keep a clearly winning evaluation");
}