#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "helpers.h"

#include "../ann_training.h"
#include "../book.h"
#include "../evaluation.h"
#include "../fen.h"
#include "../hashTable.h"
#include "../moves.h"
#include "../position.h"
#include "../search.h"
#include "../utils.h"

static void SetAnnConstantOutput(ANN *ann, double normalizedOutput)
{
	for (size_t index = 0; index < (ann->InputCount + 1) * ann->HiddenCount; index++)
		ann->WeightsInputHidden[index] = 0.0;
	for (size_t index = 0; index < (ann->HiddenCount + 1) * ann->OutputCount; index++)
		ann->WeightsHiddenOutput[index] = 0.0;

	ann->WeightsHiddenOutput[ann->HiddenCount * ann->OutputCount] = normalizedOutput * (double)(ann->HiddenCount + 1);
}

static void FillConstantPhaseOutputs(double *outputs, double normalizedOutput)
{
	for (int phase = 0; phase < ANN_PHASE_COUNT; phase++)
		outputs[phase] = normalizedOutput;
}

static void AllocateConstantAnnPhaseCollection(AnnPhaseCollection *collection, size_t hiddenCount, const double *normalizedOutputs)
{
	InitAnnPhaseCollection(collection);
	AssertAreEqualInts(0, AllocateAnnPhaseCollection(collection, hiddenCount), "phase collection allocation should succeed");
	for (int phase = 0; phase < ANN_PHASE_COUNT; phase++)
	{
		ANN *ann = GetAnnPhaseNetwork(collection, phase);
		Assert(ann != NULL, "phase network should exist");
		if (ann == NULL)
			return;
		SetAnnConstantOutput(ann, normalizedOutputs[phase]);
	}
}

TEST(AnnEncodeSquaresUseBoardOnlyInputs)
{
	double inputs[ANN_TRAINING_INPUT_COUNT] = {0};
	ReadFen("8/8/8/3p4/4P3/8/8/4K2k w - - 0 1");

	AssertAreEqualInts(64, ANN_TRAINING_INPUT_COUNT, "board-only ANN encoding should use 64 square inputs");
	AssertAreEqualInts(0, EncodeAnnInputs(&g_mainGame, inputs, ANN_TRAINING_INPUT_COUNT), __func__);
	Assert(fabs(inputs[d5] + (1.0 / 6.0)) < 1e-12, "black pawn should encode as negative pawn value");
	Assert(fabs(inputs[e4] - (1.0 / 6.0)) < 1e-12, "white pawn should encode as positive pawn value");
	Assert(fabs(inputs[e1] - 1.0) < 1e-12, "white king should encode as +1");
	Assert(fabs(inputs[h1] + 1.0) < 1e-12, "black king should encode as -1");
}

TEST(AnnEquivalentSideToMovePositionsEncodeTheSame)
{
	double whiteInputs[ANN_TRAINING_INPUT_COUNT] = {0};
	double blackInputs[ANN_TRAINING_INPUT_COUNT] = {0};
	ReadFen("8/8/8/3p4/4P3/8/8/4K2k w - - 0 1");
	AssertAreEqualInts(0, EncodeAnnInputs(&g_mainGame, whiteInputs, ANN_TRAINING_INPUT_COUNT), __func__);

	ReadFen("K2k4/8/8/3p4/4P3/8/8/8 b - - 0 1");
	AssertAreEqualInts(0, EncodeAnnInputs(&g_mainGame, blackInputs, ANN_TRAINING_INPUT_COUNT), __func__);

	for (int index = 0; index < ANN_TRAINING_INPUT_COUNT; index++)
	{
		Assert(fabs(whiteInputs[index] - blackInputs[index]) < 1e-12,
			"side-to-move perspective should make equivalent white and black positions encode identically");
	}
}

TEST(AnnNormalizeRelativeScoreUsesTanhScale)
{
	double normalized50 = NormalizeAnnRelativeScore(50);
	double normalized400 = NormalizeAnnRelativeScore(400);
	double normalized4000 = NormalizeAnnRelativeScore(4000);

	Assert(fabs(normalized50 - tanh(50.0 / (double)ANN_TRAINING_SCORE_SCALE)) < 1e-12, "50 cp should use the tanh score scale");
	Assert(fabs(normalized400 - tanh(1.0)) < 1e-12, "400 cp should map to tanh(1)");
	Assert(normalized4000 < 1.0 && normalized4000 > 0.9999, "large positive scores should saturate near +1 without clipping exactly");
	Assert(fabs(NormalizeAnnRelativeScore(-400) + normalized400) < 1e-12, "negative scores should mirror positive tanh normalization");
}

TEST(AnnPredictionScaleToCentipawns)
{
	Assert(fabs(ScaleAnnPredictionToCentipawns(NormalizeAnnRelativeScore(250)) - 250.0) < 1e-6, "normalized predictions should round-trip back to centipawns");
	Assert(fabs(ScaleAnnPredictionToCentipawns(NormalizeAnnRelativeScore(-700)) + 700.0) < 1e-6, "negative normalized predictions should round-trip back to centipawns");
}

TEST(AnnPhaseWeightsRoundTripByPhase)
{
	const char *weightsPath = "/tmp/chessmatrix_ann_phase_roundtrip_weights.txt";
	AnnPhaseCollection saved;
	AnnPhaseCollection loaded;
	double outputs[ANN_PHASE_COUNT] = {0};
	double prediction = 0.0;

	for (int phase = 0; phase < ANN_PHASE_COUNT; phase++)
		outputs[phase] = 0.05 + ((double)phase / 100.0);

	AllocateConstantAnnPhaseCollection(&saved, 4, outputs);
	InitAnnPhaseCollection(&loaded);

	AssertAreEqualInts(0, SaveAnnPhaseWeights(&saved, weightsPath), __func__);
	AssertAreEqualInts(0, LoadAnnPhaseWeights(&loaded, weightsPath), __func__);

	ReadFen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
	AssertAreEqualInts(0, GetAnnPhaseIndex(&g_mainGame), "bare kings should map to phase 0");
	AssertAreEqualInts(0, PredictAnnForPhase(&loaded, &g_mainGame, &prediction), __func__);
	Assert(fabs(prediction - outputs[0]) < 1e-9, "phase 0 prediction should survive round-trip save/load");

	ReadFen("4k3/8/8/8/8/8/4Q3/4K3 w - - 0 1");
	AssertAreEqualInts(4, GetAnnPhaseIndex(&g_mainGame), "single-queen position should map to phase 4");
	AssertAreEqualInts(0, PredictAnnForPhase(&loaded, &g_mainGame, &prediction), __func__);
	Assert(fabs(prediction - outputs[4]) < 1e-9, "phase 4 prediction should survive round-trip save/load");

	StartPosition();
	AssertAreEqualInts(24, GetAnnPhaseIndex(&g_mainGame), "start position should map to phase 24");
	AssertAreEqualInts(0, PredictAnnForPhase(&loaded, &g_mainGame, &prediction), __func__);
	Assert(fabs(prediction - outputs[24]) < 1e-9, "phase 24 prediction should survive round-trip save/load");

	remove(weightsPath);
	FreeAnnPhaseCollection(&loaded);
	FreeAnnPhaseCollection(&saved);
}

TEST(AnnTrainingOptionsDefaultUpdatesPerPosition)
{
	AnnTrainingOptions options;
	InitAnnTrainingOptions(&options);

	AssertAreEqualInts(4, options.UpdatesPerPosition, "trainer should do multiple updates per position by default");
}

TEST(AnnTrainingOptionsDefaultValidationDepth)
{
	AnnTrainingOptions options;
	InitAnnTrainingOptions(&options);

	AssertAreEqualInts(ANN_TRAINING_DEFAULT_VALIDATION_DEPTH, options.ValidationDepth, "trainer should build validation targets at a fixed default depth");
}

TEST(AnnTrainingOptionsDefaultAdaptiveLearnRate)
{
	AnnTrainingOptions options;
	InitAnnTrainingOptions(&options);

	Assert(options.AdaptiveLearnRate, "adaptive learning rate should be enabled by default");
	AssertAreEqualInts(ANN_TRAINING_DEFAULT_LEARN_RATE_PATIENCE, options.LearnRatePatience, "adaptive learning rate should use the default patience");
	Assert(fabs(options.LearnRateMin - ANN_TRAINING_DEFAULT_LEARN_RATE_MIN) < 1e-12, "adaptive learning rate should use the default floor");
	Assert(fabs(options.LearnRateDecayFactor - ANN_TRAINING_DEFAULT_LEARN_RATE_DECAY_FACTOR) < 1e-12, "adaptive learning rate should use the default decay factor");
	Assert(fabs(options.LearnRateImprovementEpsilon - ANN_TRAINING_DEFAULT_LEARN_RATE_IMPROVEMENT_EPSILON) < 1e-12, "adaptive learning rate should use the default improvement epsilon");
}

TEST(AnnLearningRateSchedulerWaitsForPatienceThenDecays)
{
	AnnTrainingOptions options;
	AnnLearningRateScheduler scheduler;

	InitAnnTrainingOptions(&options);
	options.AdaptiveLearnRate = true;
	options.LearnRate = 0.05;
	options.LearnRateMin = 0.01;
	options.LearnRateDecayFactor = 0.5;
	options.LearnRatePatience = 2;
	options.LearnRateImprovementEpsilon = 0.001;
	InitAnnLearningRateScheduler(&scheduler, &options);

	AssertNot(UpdateAnnLearningRateScheduler(&scheduler, 0.2000), "the first validation sample should only seed the scheduler");
	Assert(fabs(scheduler.CurrentLearnRate - 0.05) < 1e-12, "the seeded scheduler should keep the initial learning rate");
	AssertAreEqualInts(0, scheduler.PlateauCount, "the first validation sample should not count as a plateau");

	AssertNot(UpdateAnnLearningRateScheduler(&scheduler, 0.2005), "the first plateau sample should not decay before patience is exhausted");
	AssertAreEqualInts(1, scheduler.PlateauCount, "one plateau sample should increment the plateau counter");

	Assert(UpdateAnnLearningRateScheduler(&scheduler, 0.2004), "the scheduler should decay after the patience threshold is reached");
	Assert(fabs(scheduler.CurrentLearnRate - 0.025) < 1e-12, "the scheduler should decay by the configured factor");
	AssertAreEqualInts(0, scheduler.PlateauCount, "a decay step should reset the plateau counter");
}

TEST(AnnLearningRateSchedulerCanBeDisabled)
{
	AnnTrainingOptions options;
	AnnLearningRateScheduler scheduler;

	InitAnnTrainingOptions(&options);
	options.AdaptiveLearnRate = false;
	options.LearnRate = 0.05;
	options.LearnRateMin = 0.01;
	options.LearnRateDecayFactor = 0.5;
	options.LearnRatePatience = 1;
	options.LearnRateImprovementEpsilon = 0.001;
	InitAnnLearningRateScheduler(&scheduler, &options);

	AssertNot(UpdateAnnLearningRateScheduler(&scheduler, 0.2000), "the first validation sample should only seed the disabled scheduler");
	AssertNot(UpdateAnnLearningRateScheduler(&scheduler, 0.2005), "a disabled scheduler should never decay the learning rate");
	Assert(fabs(scheduler.CurrentLearnRate - 0.05) < 1e-12, "disabling the scheduler should preserve the fixed learning rate");
}

TEST(AnnLearningRateSchedulerClampsAtConfiguredMinimum)
{
	AnnTrainingOptions options;
	AnnLearningRateScheduler scheduler;

	InitAnnTrainingOptions(&options);
	options.AdaptiveLearnRate = true;
	options.LearnRate = 0.02;
	options.LearnRateMin = 0.015;
	options.LearnRateDecayFactor = 0.5;
	options.LearnRatePatience = 1;
	options.LearnRateImprovementEpsilon = 0.001;
	InitAnnLearningRateScheduler(&scheduler, &options);

	AssertNot(UpdateAnnLearningRateScheduler(&scheduler, 0.2000), "the first validation sample should seed the scheduler before clamping is relevant");
	Assert(UpdateAnnLearningRateScheduler(&scheduler, 0.2005), "the scheduler should still report a decay when it clamps at the minimum rate");
	Assert(fabs(scheduler.CurrentLearnRate - 0.015) < 1e-12, "the scheduler should not decay below the configured floor");
}

TEST(AnnTrainingProgressStatsTrackAverages)
{
	AnnTrainingProgressStats stats;
	InitAnnTrainingProgressStats(&stats);
	AddAnnTrainingProgressSample(&stats, 0.20, 0.50);
	AddAnnTrainingProgressSample(&stats, -0.10, 0.10);

	AssertAreEqualInts(2, stats.TotalSamples, "stats should count total samples");
	AssertAreEqualInts(2, stats.WindowSamples, "stats should count window samples");
	Assert(fabs(GetAnnTrainingAverageAbsError(&stats) - 0.25) < 1e-12, "average absolute error should match the inserted samples");
	Assert(fabs(GetAnnTrainingAverageSquaredError(&stats) - 0.065) < 1e-12, "average squared error should match the inserted samples");
	Assert(fabs(GetAnnTrainingWindowAverageAbsError(&stats) - 0.25) < 1e-12, "window absolute error should match before reset");
	Assert(fabs(GetAnnTrainingWindowAverageSquaredError(&stats) - 0.065) < 1e-12, "window squared error should match before reset");
	Assert(fabs(stats.LastAbsoluteError - 0.20) < 1e-12, "last absolute error should reflect the last sample");
}

TEST(AnnTrainingProgressStatsResetWindowKeepsTotals)
{
	AnnTrainingProgressStats stats;
	InitAnnTrainingProgressStats(&stats);
	AddAnnTrainingProgressSample(&stats, 0.00, 0.50);
	AddAnnTrainingProgressSample(&stats, 0.20, -0.10);
	ResetAnnTrainingProgressWindow(&stats);
	AddAnnTrainingProgressSample(&stats, 0.10, 0.20);

	AssertAreEqualInts(3, stats.TotalSamples, "resetting the window should not reset total sample count");
	AssertAreEqualInts(1, stats.WindowSamples, "resetting the window should only leave the new sample in the window");
	Assert(fabs(GetAnnTrainingAverageAbsError(&stats) - ((0.50 + 0.30 + 0.10) / 3.0)) < 1e-12, "total absolute error should keep all samples");
	Assert(fabs(GetAnnTrainingWindowAverageAbsError(&stats) - 0.10) < 1e-12, "window absolute error should only use post-reset samples");
	Assert(fabs(GetAnnTrainingWindowAverageSquaredError(&stats) - 0.01) < 1e-12, "window squared error should only use post-reset samples");
}

TEST(AnnValidationStatsTrackAverages)
{
	AnnValidationStats stats;
	InitAnnValidationStats(&stats);
	AddAnnValidationSample(&stats, 0.10, 0.40);
	AddAnnValidationSample(&stats, -0.20, 0.10);

	AssertAreEqualInts(2, stats.SampleCount, "validation stats should count samples");
	Assert(fabs(GetAnnValidationAverageAbsError(&stats) - 0.30) < 1e-12, "validation absolute error average should be correct");
	Assert(fabs(GetAnnValidationAverageSquaredError(&stats) - 0.09) < 1e-12, "validation squared error average should be correct");
	Assert(fabs(stats.LastAbsoluteError - 0.30) < 1e-12, "validation stats should store the last absolute error");
}

TEST(AnnTrainingProgressCsvRowIncludesValidationMetrics)
{
	AnnTrainingProgressStats progress;
	AnnValidationStats validation;
	char row[512];

	InitAnnTrainingProgressStats(&progress);
	AddAnnTrainingProgressSample(&progress, 0.20, 0.50);
	AddAnnTrainingProgressSample(&progress, -0.10, 0.10);
	InitAnnValidationStats(&validation);
	AddAnnValidationSample(&validation, 0.00, 0.30);

	Assert(StartsWith((char *)GetAnnTrainingProgressCsvHeader(), "kind,game,roots,window_roots,"), "csv header should start with the expected column list");
	Assert(Contains((char *)GetAnnTrainingProgressCsvHeader(), ",roots_per_sec,learn_rate\n"), "csv header should include the learn rate column");
	AssertAreEqualInts(0, FormatAnnTrainingProgressCsvRow(row, sizeof(row), "progress", 3, 1500, 20.0, 0.025, &progress, &validation), __func__);
	Assert(StartsWith(row, "progress,3,2,2,"), "csv row should start with the row kind and counters");
	Assert(Contains(row, ",1,0.300000,0.090000,0.300000,1500,20.0,0.025000\n"), "csv row should include validation metrics, elapsed time, speed, and learn rate");
}

TEST(AnnResolveTrainingMoveMapsCastleFromHashCoordinates)
{
	Move legalMoves[MAX_MOVES];
	Move desiredMove = {0};
	Move resolvedMove = {0};
	ReadFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
	int moveCount = ValidMoves(legalMoves);

	desiredMove.From = e1;
	desiredMove.To = g1;
	desiredMove.MoveInfo = PlainMove;
	desiredMove.PieceIdx = 255;

	AssertAreEqualInts(0, ResolveAnnTrainingMove(desiredMove, legalMoves, moveCount, &resolvedMove), __func__);
	AssertAreEqualInts(CastleShort, resolvedMove.MoveInfo, "castle should resolve to the legal castle move");
	Assert(resolvedMove.PieceIdx != 255, "resolved move should carry a usable piece index for DoMove");
}

TEST(AnnRandomOpeningPliesAdvanceGame)
{
	Move moves[MAX_MOVES];
	StartPosition();
	srand(1);

	Assert(ApplyRandomOpeningPlies(&g_mainGame, 2), __func__);
	AssertAreEqualInts(2, g_mainGame.PositionHistoryLength, "two random opening plies should advance game history twice");
	AssertAreEqualInts(WHITE, g_mainGame.Side, "two plies should return the turn to white");
	Assert(ValidMoves(moves) > 0, "randomized opening position should remain legal");
}

TEST(AnnTrainPhasePositionUpdatesOnlyMatchingNetwork)
{
	AnnPhaseCollection collection;
	double outputs[ANN_PHASE_COUNT] = {0};
	double beforeEndgame = 0.0;
	double afterEndgame = 0.0;
	double beforeOpening = 0.0;
	double afterOpening = 0.0;

	FillConstantPhaseOutputs(outputs, 0.0);
	AllocateConstantAnnPhaseCollection(&collection, 8, outputs);
	SetAnnPhaseCollectionLearnRate(&collection, 0.05);

	ReadFen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
	AssertAreEqualInts(0, GetAnnPhaseIndex(&g_mainGame), "training position should use phase 0");
	AssertAreEqualInts(0, PredictAnnForPhase(&collection, &g_mainGame, &beforeEndgame), __func__);

	StartPosition();
	AssertAreEqualInts(24, GetAnnPhaseIndex(&g_mainGame), "reference position should use phase 24");
	AssertAreEqualInts(0, PredictAnnForPhase(&collection, &g_mainGame, &beforeOpening), __func__);

	ReadFen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
	for (int i = 0; i < 200; i++)
		AssertAreEqualInts(0, TrainAnnForPhase(&collection, &g_mainGame, 300, NULL, NULL), "phase-aware training step should succeed");
	AssertAreEqualInts(0, PredictAnnForPhase(&collection, &g_mainGame, &afterEndgame), __func__);

	StartPosition();
	AssertAreEqualInts(0, PredictAnnForPhase(&collection, &g_mainGame, &afterOpening), __func__);

	Assert(afterEndgame > beforeEndgame, "training should move the matching phase network toward the target");
	Assert(fabs(afterOpening - beforeOpening) < 1e-12, "training one phase should leave other phase networks untouched");

	FreeAnnPhaseCollection(&collection);
}

TEST(AnnTrainSingleSampleReducesError)
{
	ANN *ann;
	double inputs[ANN_TRAINING_INPUT_COUNT] = {0};
	double beforeError;
	double afterError;
	double target = NormalizeAnnRelativeScore(300);

	ReadFen("8/8/8/3p4/4P3/8/8/4K2k w - - 0 1");
	srand(1);
	ann = NewAnn(ANN_TRAINING_INPUT_COUNT, 12, ANN_TRAINING_OUTPUT_COUNT);
	Assert(ann != NULL, "ann allocation failed");
	if (ann == NULL)
		return;
	ann->LearnRate = 0.05;

	AssertAreEqualInts(0, EncodeAnnInputs(&g_mainGame, inputs, ANN_TRAINING_INPUT_COUNT), __func__);
	AssertAreEqualInts(0, Compute(ann, inputs, ANN_TRAINING_INPUT_COUNT), "compute before training should succeed");
	beforeError = fabs(target - ann->Output[0]);

	for (int i = 0; i < 200; i++)
		AssertAreEqualInts(0, TrainAnnOnPosition(ann, &g_mainGame, 300, NULL, NULL), "training step should succeed");

	AssertAreEqualInts(0, Compute(ann, inputs, ANN_TRAINING_INPUT_COUNT), "compute after training should succeed");
	afterError = fabs(target - ann->Output[0]);
	Assert(afterError < beforeError, "repeated online training should reduce the sample error");

	FreeAnn(ann);
}

SEARCH_TEST(SearchDoesNotExposeScoreForForcedMove)
{
	bool savedOwnBook = OwnBook;
	OwnBook = false;
	ReadFen("k7/R7/1K6/8/8/8/2P5/8 b - - 0 1");
	SetSearchDefaults();
	g_topSearchParams.MaxDepth = 1;
	g_topSearchParams.MoveTime = 0;
	g_topSearchParams.TimeControl = false;

	Search(false);
	AssertNot(g_topSearchParams.HasSearchScore, "forced-move shortcut should not report a searched score");

	OwnBook = savedOwnBook;
}

SEARCH_TEST(SearchStoresScoreAfterCompletedDepth)
{
	bool savedOwnBook = OwnBook;
	OwnBook = false;
	StartPosition();
	SetSearchDefaults();
	g_topSearchParams.MaxDepth = 1;
	g_topSearchParams.MoveTime = 0;
	g_topSearchParams.TimeControl = false;

	Search(false);
	Assert(g_topSearchParams.HasSearchScore, "searched positions should expose the last completed root score");

	OwnBook = savedOwnBook;
}

SEARCH_TEST(AnnValidationSetPreservesCurrentPosition)
{
	AnnPhaseCollection collection;
	AnnTrainingOptions options;
	AnnValidationCache cache;
	AnnValidationStats validation;
	double outputs[ANN_PHASE_COUNT] = {0};
	char beforeFen[256];
	char afterFen[256];
	U64 beforeHash;
	int beforeHistoryLength;
	bool savedOwnBook = OwnBook;

	OwnBook = false;
	ReadFen("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/2N5/PPPP1PPP/R1BQKBNR w KQkq - 2 3");
	WriteFen(beforeFen);
	beforeHash = g_mainGame.Hash;
	beforeHistoryLength = g_mainGame.PositionHistoryLength;
	InitAnnTrainingOptions(&options);
	options.ValidationDepth = 1;
	InitAnnValidationCache(&cache);
	FillConstantPhaseOutputs(outputs, 0.0);
	AllocateConstantAnnPhaseCollection(&collection, 8, outputs);

	AssertAreEqualInts(0, BuildAnnValidationCache(&options, &cache), __func__);
	Assert(cache.SampleCount > 4, "validation cache should include the expanded fixed validation set");
	AssertAreEqualInts(0, EvaluateAnnValidationSet(&collection, &cache, &validation), __func__);
	WriteFen(afterFen);
	AssertAreEqual(beforeFen, afterFen, "validation should restore the original board position");
	AssertAreEqualLongs(beforeHash, g_mainGame.Hash, "validation should restore the original hash");
	AssertAreEqualInts(beforeHistoryLength, g_mainGame.PositionHistoryLength, "validation should restore the original position history length");
	AssertAreEqualInts(cache.SampleCount, validation.SampleCount, "validation should evaluate the full cached validation set");

	FreeAnnPhaseCollection(&collection);
	OwnBook = savedOwnBook;
}

SEARCH_TEST(AnnValidationCacheBuildsStableTargets)
{
	AnnTrainingOptions options;
	AnnValidationCache firstCache;
	AnnValidationCache secondCache;
	bool savedOwnBook = OwnBook;

	OwnBook = false;
	InitAnnTrainingOptions(&options);
	options.ValidationDepth = 1;
	InitAnnValidationCache(&firstCache);
	InitAnnValidationCache(&secondCache);

	AssertAreEqualInts(0, BuildAnnValidationCache(&options, &firstCache), __func__);
	ReadFen("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/2N5/PPPP1PPP/R1BQKBNR w KQkq - 2 3");
	ClearHashTable();
	AssertAreEqualInts(0, BuildAnnValidationCache(&options, &secondCache), __func__);
	AssertAreEqualInts(firstCache.SampleCount, secondCache.SampleCount, "stable validation builds should keep the same sample count");
	Assert(firstCache.SampleCount > 0, "validation cache should contain samples");
	for (int index = 0; index < firstCache.SampleCount; index++)
	{
		AssertAreEqualInts(firstCache.Samples[index].Phase, secondCache.Samples[index].Phase,
			"validation cache rebuilds should preserve per-sample phase selection");
		Assert(fabs(firstCache.Samples[index].Target - secondCache.Samples[index].Target) < 1e-12,
			"validation targets should be repeatable across cache rebuilds");
	}

	OwnBook = savedOwnBook;
}