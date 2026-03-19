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

SEARCH_TEST(BestMoveTestBlackCaptureBishop)
{
	AssertBestMove(4, __func__, "r1bqk2r/ppp1bppp/2n1pn2/3p4/2BP1B2/2N1PN2/PPP2PPP/R2QK2R b KQkq - 2 6", "d5c4");
}

SEARCH_TEST(TestWhiteMateIn2)
{
	char *fen = "5k2/8/2Q5/3R4/8/8/8/4K3 w - - 2 1";
	AssertBestMove(5, __func__, fen, "d5d7");
	AssertBestMoveTimed(1000, __func__, fen, "d5d7");
}

SEARCH_TEST(BlackMatesIn5Deeping)
{
	char *fen = "1k2r3/pP3pp1/8/3P1B1p/5q2/N1P2b2/PP3Pp1/R5K1 b - - 0 1";
	AssertBestMoveTimed(2000, __func__, fen, "f4h4");
}

SEARCH_TEST(BestMoveByWhite1)
{
	char *fen = "r1bqkb1r/ppp1pppp/2npn3/4P3/2P5/2N2NP1/PP1P1P1P/R1BQKB1R w KQkq - 1 1";
	AssertBestMove(7, __func__, fen, "d2d4");
}

void BestMoveByBlack2()
{
	char *fen = "r1r5/1p6/2kpQ3/3p4/n2P4/4P3/3q1PPP/R4RK1 b - - 0 21";
	AssertBestMove(7, __func__, fen, "a4c3");
}

void BestMoveByBlack3()
{
	char *fen = "8/kp6/8/3p4/3PnQ2/4P1P1/r2q1P1P/5RK1 b - - 2 27";
	AssertBestMove(7, __func__, fen, "d2e2");
}

SEARCH_TEST(BestMoveByBlack1)
{
	char *fen = "r1bq2k1/p1p2pp1/2p2n1p/3pr3/7B/P1PBPQ2/5PPP/R4RK1 b - - 0 1";
	AssertBestMove(6, __func__, fen, "g7g5");
}

SEARCH_TEST(BestMoveByBlack4)
{
	char *fen = "r1b2r2/2q2pk1/2pb3p/pp2pNpn/4Pn2/P1NB2BP/1PP1QPP1/R4RK1 b - - 0 1";
	AssertBestMove(5, __func__, fen, "c8f5");
}

SEARCH_TEST(BestMoveByBlack5)
{
	char *fen = "r2qk2r/1b3pp1/pb2p2p/Rp2P3/2pPB3/2P2N2/2Q2PPP/2B2RK1 b - - 0 1";
	AssertBestMove(6, __func__, fen, "b7e4");
}

SEARCH_TEST(BestMoveByBlack6)
{
	char *fen = "8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - 0 1";
	AssertBestMove(15, __func__, fen, "b3b2");
}

SEARCH_TEST(BestMoveByWhite3)
{
	char *fen = "r4rk1/p7/1p1N3p/3nPppb/3n4/3B3P/PP1B2K1/R4R2 w - - 0 1";
	AssertBestMove(5, __func__, fen, "d3c4");
}

SEARCH_TEST(RookSacrificeByWhite)
{
	char *fen = "r2q2k1/p4p1p/1rp3bB/3p4/3P1Q2/RP3P2/1KP5/4R3 w - - 3 47";
	AssertBestMoveTimed(3000, __func__, fen, "e1e8");
}

SEARCH_TEST(BlackMatesIn5a)
{
	char *fen = "6k1/3b3r/1p1p4/p1n2p2/1PPNpP1q/P3Q1p1/1R1RB1P1/5K2 b - -";
	AssertBestMoveTimed(500, __func__, fen, "h4f4");
}

SEARCH_TEST(WhiteMatesIn5b)
{
	char *fen = "2q1nk1r/4Rp2/1ppp1P2/6Pp/3p1B2/3P3P/PPP1Q3/6K1 w";
	AssertBestMove(11, __func__, fen, "e7e8");
}

SEARCH_TEST(WhiteMatesIn7)
{
	char *fen = "rn3rk1/pbppq1pp/1p2pb2/4N2Q/3PN3/3B4/PPP2PPP/R3K2R w KQ - 7 11";
	AssertBestMove(15, __func__, fen, "h5h7");
}

SEARCH_TEST(EngineMated)
{
	char *fen = "rn3rk1/pbppq1pQ/1p2pb2/4N3/3PN3/3B4/PPP2PPP/R3K2R b KQ - 0 11";
	AssertBestMove(8, __func__, fen, "g8h7");
}

void DeepTest()
{
	printf("%s\n", __func__);
	char *fen = "r1b1k2r/ppppnppp/2n2q2/2b5/3NP3/2P1B3/PP3PPP/RN1QKB1R w KQkq - 0 1";
	AssertBestMove(7, __func__, fen, "b1d2");
}