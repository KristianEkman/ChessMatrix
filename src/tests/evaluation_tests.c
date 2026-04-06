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

short PawnPhalanx(int square, Game *game);
short EndgameKingPawnTropism(int square, Game *game);
short RookBehindPassedPawn(int square, Game *game);
short SimplificationBonus(Game *game);

static Piece *FindPieceAt(int side01, int square)
{
	for (int i = 0; i < 16; i++)
	{
		Piece *piece = &g_mainGame.Pieces[side01][i];
		if (!piece->Off && piece->SquareIndex == square)
			return piece;
	}

	return NULL;
}

TEST(UnstoppablePassedPawnEvalTest)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/1P6/8/8/8/4K3 w - - 0 1");
	Assert(GetEval(&g_mainGame) < -300, "Advanced unstoppable passer should evaluate as clearly winning in a king-and-pawn ending");
}

TEST(PassedPawnRaceBonusNeedsClearPath)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/1P6/1P6/8/8/8/4K3 w - - 0 1");

	AssertAreEqualInts(0, PassedPawnRaceBonus(b5, &g_mainGame), "Passed-pawn race bonus should be zero when a friendly pawn blocks the advance path");
}

TEST(PassedPawnRaceBonusRewardsExtraKingDistance)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/1P6/8/8/8/4K3 w - - 0 1");
	short nearBonus = PassedPawnRaceBonus(b5, &g_mainGame);

	ReadFen("7k/8/8/1P6/8/8/8/4K3 w - - 0 1");
	short farBonus = PassedPawnRaceBonus(b5, &g_mainGame);

	Assert(nearBonus > 0, "Expected the closer black king to still be outside the pawn square");
	Assert(farBonus > nearBonus, "Passed-pawn race bonus should increase when the enemy king is farther away");
}

TEST(PawnPhalanxRewardsAdjacentAdvancedPawns)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/6PP/8/8/4K3 w - - 0 1");
	AssertAreEqualInts(8, PawnPhalanx(g4, &g_mainGame), "Advanced adjacent pawns should receive a phalanx bonus");
	AssertAreEqualInts(8, PawnPhalanx(h4, &g_mainGame), "Edge pawn in a phalanx should receive the same bonus");

	ReadFen("4k3/8/8/8/6P1/7P/8/4K3 w - - 0 1");
	AssertAreEqualInts(0, PawnPhalanx(g4, &g_mainGame), "Separated pawns should not receive a phalanx bonus");
	AssertAreEqualInts(0, PawnPhalanx(h3, &g_mainGame), "Diagonal support alone should not count as a phalanx");
}

TEST(EndgameKingTropismRewardsApproachingEnemyPawns)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/7p/8/4K3/8/8/8 w - - 0 1");
	short activeKing = EndgameKingPawnTropism(e4, &g_mainGame);

	ReadFen("4k3/8/7p/8/8/8/8/K7 w - - 0 1");
	short passiveKing = EndgameKingPawnTropism(a1, &g_mainGame);

	Assert(activeKing > passiveKing, "Endgame king activity should reward moving closer to enemy pawns");

	ReadFen("4k3/8/7p/8/4K3/8/8/4Q3 w - - 0 1");
	AssertAreEqualInts(0, EndgameKingPawnTropism(e4, &g_mainGame), "King tropism bonus should stay disabled while major pieces remain on the board");
}

TEST(RookBehindOwnPassedPawnGetsBonus)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/P7/8/8/8/R3K3 w - - 0 1");
	short behind = RookBehindPassedPawn(a1, &g_mainGame);
	Assert(behind > 0, "Rook behind own passed pawn should earn a bonus");

	ReadFen("4k3/8/R7/P7/8/8/8/4K3 w - - 0 1");
	short inFront = RookBehindPassedPawn(a6, &g_mainGame);
	AssertAreEqualInts(0, inFront, "Rook in front of own passed pawn should not earn a bonus");

	ReadFen("4k3/8/8/8/8/8/8/R3K3 w - - 0 1");
	short noPasser = RookBehindPassedPawn(a1, &g_mainGame);
	AssertAreEqualInts(0, noPasser, "Rook with no passed pawn on its file should not earn a bonus");
}

TEST(SimplificationBonusRewardsTradesWhenAhead)
{
	printf("%s\n", __func__);
	ReadFen("rnbqkbnr/ppp1pppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	short fullBoard = SimplificationBonus(&g_mainGame);
	AssertAreEqualInts(0, fullBoard, "No simplification bonus when all pieces are on the board");

	ReadFen("4k3/ppp1pppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1");
	short pawnEndgame = SimplificationBonus(&g_mainGame);
	Assert(pawnEndgame < 0, "When white is ahead, simplification bonus should favor white (negative)");

	ReadFen("4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1");
	short equalMaterial = SimplificationBonus(&g_mainGame);
	AssertAreEqualInts(0, equalMaterial, "No simplification bonus when material is equal");
}

TEST(EndgameMoveCountDoesNotTriggerOpeningPenalty)
{
	printf("%s\n", __func__);
	ReadFen("4k3/8/8/8/8/8/8/R3K3 w - - 0 1");
	short baseEval = GetEval(&g_mainGame);
	Piece *rook = FindPieceAt(0, a1);

	Assert(rook != NULL, "Expected to find the white rook on a1");
	rook->MoveCount = 2;

	AssertAreEqualInts(baseEval, GetEval(&g_mainGame), "Sparse endgames loaded from FEN should not get opening move-count penalties");
}

TEST(QueenEarlyDevelopmentPenaltyAppliesToColoredQueens)
{
	printf("%s\n", __func__);
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	short baseEval = GetEval(&g_mainGame);
	Piece *queen = FindPieceAt(0, d1);

	Assert(queen != NULL, "Expected to find the white queen on d1");
	queen->MoveCount = 1;

	AssertAreEqualInts(baseEval + QUEEN_EARLY, GetEval(&g_mainGame), "A white queen that has moved in the opening should receive the queen-early penalty");
}

TEST(KingMoveOrderingUsesInterpolatedGamePhase)
{
	printf("%s\n", __func__);
	ReadFen("r1b1k1n1/8/8/8/8/8/8/4K3 w - - 0 1");

	Move move = ParseMove("e1e2", KingMove);
	int gamePhase = GetGamePhase(&g_mainGame);
	int maxGamePhase = 24;
	int middleGameDelta = KingPositionValueMatrix[0][0][e2] - KingPositionValueMatrix[0][0][e1];
	int endGameDelta = KingPositionValueMatrix[1][0][e2] - KingPositionValueMatrix[1][0][e1];
	int expectedKingDelta = (middleGameDelta * gamePhase + endGameDelta * (maxGamePhase - gamePhase)) / maxGamePhase;
	int expectedScore = g_mainGame.Material[0] + g_mainGame.Material[1] + expectedKingDelta;

	Assert(gamePhase > 0 && gamePhase < maxGamePhase, "Expected the test position to stay in a mixed game phase");
	AssertAreEqualInts(expectedScore, GetMoveOrderingScore(move, &g_mainGame), "King move ordering should blend middle-game and endgame king tables by game phase");
}

TEST(BishopMobilityTest)
{
	printf("%s\n", __func__);

	ReadFen("4k3/8/8/8/3B4/8/8/4K3 w - - 0 1");
	AssertAreEqualInts(13 * BISHOP_MOBILITY, BishopMobility(d4, &g_mainGame), "Open bishop mobility score mismatch");

	ReadFen("4k3/8/8/2P1P3/3B4/2P1P3/8/4K3 w - - 0 1");
	AssertAreEqualInts(0, BishopMobility(d4, &g_mainGame), "Blocked bishop mobility score mismatch");
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
	AssertAreEqualInts(DOUBLE_PAWN, score, "Double pawns score missmatch");
	AssertAreEqualInts(0, DoublePawns(8, &g_mainGame, PAWN | WHITE), "Double pawns score missmatch");
}

TEST(OpenRookFileTest)
{
	printf("%s\n", __func__);

	ReadFen("4k3/8/8/8/8/8/8/R3K3 w - - 0 1");
	AssertAreEqualInts(OPEN_ROOK_FILE, OpenRookFile(a1, &g_mainGame, ROOK | WHITE), "Open rook file score mismatch");

	ReadFen("4k3/p7/8/8/8/8/8/R3K3 w - - 0 1");
	AssertAreEqualInts(SEMI_OPEN_FILE, OpenRookFile(a1, &g_mainGame, ROOK | WHITE), "Semi-open rook file score mismatch");

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
	AssertAreEqualInts(60, whiteScore, "Not the expected exposure score for white");
	AssertAreEqualInts(60, blackScore, "Not the expected exposure score for black");
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