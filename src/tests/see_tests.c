#include <stdio.h>

#include "helpers.h"

#include "../evaluation.h"
#include "../fen.h"
#include "../moves.h"

TEST(StaticExchangeEvaluationWinsUnprotectedPawn)
{
    printf("%s\n", __func__);
    ReadFen("4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1");

    Move capture = ParseMove("e4d5", PlainMove);
    short see = StaticExchangeEvaluation(capture, &g_mainGame);

    AssertAreEqualInts(MATERIAL_P, see, "SEE should score a free pawn capture as +100");
}

TEST(StaticExchangeEvaluationRejectsLosingQueenCapture)
{
    printf("%s\n", __func__);
    ReadFen("3r2k1/8/8/3p4/8/8/8/3Q2K1 w - - 0 1");

    Move capture = ParseMove("d1d5", PlainMove);
    short see = StaticExchangeEvaluation(capture, &g_mainGame);

    AssertAreEqualInts(MATERIAL_P - MATERIAL_Q, see, "SEE should score Qxd5 as losing the queen after ...Rxd5");
}

TEST(StaticExchangeEvaluationCountsPromotionGainOnCapture)
{
    printf("%s\n", __func__);
    ReadFen("k6r/6P1/8/8/8/8/8/K7 w - - 0 1");

    Move capture = ParseMove("g7h8q", PlainMove);
    short see = StaticExchangeEvaluation(capture, &g_mainGame);

    AssertAreEqualInts(MATERIAL_R + MATERIAL_Q - MATERIAL_P, see, "SEE should include the promotion gain when a pawn captures and queens");
}

TEST(StaticExchangeEvaluationCountsPromotionGainBeforeRecapture)
{
    printf("%s\n", __func__);
    ReadFen("k6r/6P1/5b2/8/8/8/8/K7 w - - 0 1");

    Move capture = ParseMove("g7h8q", PlainMove);
    short see = StaticExchangeEvaluation(capture, &g_mainGame);

    AssertAreEqualInts(MATERIAL_R + MATERIAL_Q - MATERIAL_P - MATERIAL_Q, see, "SEE should include promotion gain before ...Bxh8 recaptures the new queen");
}

TEST(StaticExchangeEvaluationWinsUnprotectedEnPassant)
{
    printf("%s\n", __func__);
    ReadFen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");

    Move capture = ParseMove("e5d6", EnPassantCapture);
    short see = StaticExchangeEvaluation(capture, &g_mainGame);

    AssertAreEqualInts(MATERIAL_P, see, "SEE should score a free en passant capture as +100");
}

TEST(StaticExchangeEvaluationTradesEvenAfterEnPassantRecapture)
{
    printf("%s\n", __func__);
    ReadFen("3rk3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");

    Move capture = ParseMove("e5d6", EnPassantCapture);
    short see = StaticExchangeEvaluation(capture, &g_mainGame);

    AssertAreEqualInts(0, see, "SEE should see e.p. as equal when ...Rxd6 recaptures immediately");
}
