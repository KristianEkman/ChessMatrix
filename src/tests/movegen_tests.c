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

TEST(ValidMovesPromotionCaptureAndCastling)
{
	printf("%s\n", __func__);
	char *fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
	Move moves[MAX_MOVES];
	ReadFen(fen);
	int count = ValidMoves(moves);
	AssertAreEqualInts(44, count, "Moves count missmatch");
	Move expectedMove;
	expectedMove.From = 4;
	expectedMove.To = 6;
	expectedMove.MoveInfo = CastleShort;
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
}

TEST(LongCastling)
{
	printf("%s\n", __func__);
	char *fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
	Move moves[MAX_MOVES];
	ReadFen(fen);
	int count = ValidMoves(moves);
	AssertAreEqualInts(48, count, "Moves count missmatch");
	Move expectedMove;
	expectedMove.From = 4;
	expectedMove.To = 2;
	expectedMove.MoveInfo = CastleLong;
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
}

TEST(EnPassantFromFenTest)
{
	printf("%s\n", __func__);
	char *fen = "8/5k2/8/3Pp3/8/8/8/4K3 w - e6 0 3";
	ReadFen(fen);
	Move moves[MAX_MOVES];
	int count = ValidMoves(moves);
	Move expectedMove = ParseMove("d5e6", EnPassantCapture);
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
	int startGameScore = TotalMaterial(&g_mainGame);
	AssertNot(MakePlayerMove("d5e6").Invalid, "Invalid move");
	AssertAreEqualInts(startGameScore - 100, TotalMaterial(&g_mainGame), "Material should decrease by 100");
}

TEST(EnPassantAfterMove)
{
	printf("%s\n", __func__);
	char *fen = "4k3/4p3/8/3P4/8/8/8/4K3 b - - 0 1";
	ReadFen(fen);
	AssertNot(MakePlayerMove("e7e5").Invalid, "Move was not valid");

	Move moves[MAX_MOVES];
	int count = ValidMoves(moves);
	Move expectedMove = ParseMove("d5e6", EnPassantCapture);
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
}

TEST(BlackCastlingRightsAfterKingMove)
{
	printf("%s\n", __func__);
	char *fen = "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/P4N2/1PPP1PPP/RNBQK2R w KQkq - 1 5";
	ReadFen(fen);
	AssertNot(MakePlayerMove("e1f1").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("e8f8").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("f1e1").Invalid, "Move was not valid");

	Move moves[MAX_MOVES];
	int count = ValidMoves(moves);
	Move expectedMove = ParseMove("e8g8", CastleShort);
	AssertNot(MovesContains(moves, count, expectedMove), "Invalid move was found");
}

TEST(WhiteCastlingRightsAfterKingMove)
{
	printf("%s\n", __func__);
	char *fen = "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/P4N2/1PPP1PPP/RNBQK2R w KQkq - 1 5";
	ReadFen(fen);
	AssertNot(MakePlayerMove("e1f1").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("e8f8").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("f1e1").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("f8e8").Invalid, "Move was not valid");

	Move moves[MAX_MOVES];
	int count = ValidMoves(moves);
	Move expectedMove = ParseMove("e1g1", CastleShort);
	AssertNot(MovesContains(moves, count, expectedMove), "Invalid move was found");
}

TEST(CastlingRightsClearedWhenRookIsCaptured)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/8/8/q7/R3K3 b Q - 0 1");
	PlayerMove pm = MakePlayerMove("a2a1");
	AssertNot(pm.Invalid, "Move was not valid");
	AssertNot(g_mainGame.State & WhiteCanCastleLong, "White long castling rights should be cleared after the rook on a1 is captured");
}

TEST(MakePlayerMoveUsesRequestedUnderpromotion)
{
	printf("%s\n", __func__);
	ReadFen("4k3/1P6/8/8/8/8/8/4K3 w - - 0 1");

	Move moves[MAX_MOVES];
	int count = ValidMoves(moves);
	Move expectedMove = ParseMove("b7b8n", PlainMove);
	Assert(MovesContains(moves, count, expectedMove), "knight underpromotion should be generated as a legal move");

	PlayerMove pm = MakePlayerMove("b7b8n");
	AssertNot(pm.Invalid, "knight underpromotion should be accepted as a legal player move");
	AssertAreEqualInts(PromotionKnight, pm.Move.MoveInfo, "player move should preserve the requested underpromotion piece");
	AssertAreEqualInts(KNIGHT | WHITE, g_mainGame.Squares[b8], "board should contain a knight after knight underpromotion");

	UnMakePlayerMove(pm);
	AssertAreEqualInts(PAWN | WHITE, g_mainGame.Squares[b7], "undo should restore the pawn after underpromotion test");
}

TEST(PinnedMoveLegalityTest)
{
	printf("%s\n", __func__);
	ReadFen("4r1k1/8/8/8/8/8/4R3/4K3 w - - 0 1");

	PlayerMove illegal = MakePlayerMove("e2d2");
	Assert(illegal.Invalid, "pinned rook should not be allowed to move off the pin line");

	PlayerMove legal = MakePlayerMove("e2e8");
	AssertNot(legal.Invalid, "pinned rook should be allowed to capture the pinning rook");
	UnMakePlayerMove(legal);
}

TEST(CheckEvasionLegalityTest)
{
	printf("%s\n", __func__);
	ReadFen("4r1k1/8/8/8/8/8/4R3/4K3 w - - 0 1");

	Move moves[MAX_MOVES];
	int count = ValidMoves(moves);
	Assert(count > 0, "checked side should still have legal evasions in the test position");

	PlayerMove illegal = MakePlayerMove("e2f2");
	Assert(illegal.Invalid, "moves that do not answer check should be rejected");

	PlayerMove legal = MakePlayerMove("e2e3");
	AssertNot(legal.Invalid, "blocking the checking rook should stay legal");
	UnMakePlayerMove(legal);
}