#include "ann_training.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "book.h"
#include "evaluation.h"
#include "fen.h"
#include "hashTable.h"
#include "moves.h"
#include "position.h"
#include "search.h"
#include "timeControl.h"

typedef struct {
	const char *Name;
	const char *Fen;
} AnnValidationPosition;

static const AnnValidationPosition g_annValidationPositions[] = {
	{ "middlegame-1", "r1bqkb1r/ppp1pppp/2npn3/4P3/2P5/2N2NP1/PP1P1P1P/R1BQKB1R w KQkq - 1 1" },
	{ "middlegame-2", "r1bq2k1/p1p2pp1/2p2n1p/3pr3/7B/P1PBPQ2/5PPP/R4RK1 b - - 0 1" },
	// { "tactics", "r2q2k1/p4p1p/1rp3bB/3p4/3P1Q2/RP3P2/1KP5/4R3 w - - 3 47" },
	{ "endgame", "8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - 0 1" },
	{ "opening-1", "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/2N5/PPPP1PPP/R1BQKBNR w KQkq - 2 3" },
	{ "opening-2", "r1bqk2r/ppp1bppp/2n1pn2/3p4/2BP1B2/2N1PN2/PPP2PPP/R2QK2R b KQkq - 2 6" },
	{ "middlegame-3", "r1b2r2/2q2pk1/2pb3p/pp2pNpn/4Pn2/P1NB2BP/1PP1QPP1/R4RK1 b - - 0 1" },
	{ "middlegame-4", "r2qk2r/1b3pp1/pb2p2p/Rp2P3/2pPB3/2P2N2/2Q2PPP/2B2RK1 b - - 0 1" },
	{ "middlegame-5", "r4rk1/p7/1p1N3p/3nPppb/3n4/3B3P/PP1B2K1/R4R2 w - - 0 1" },
	// { "tactics-2", "rn3rk1/pbppq1pp/1p2pb2/4N2Q/3PN3/3B4/PPP2PPP/R3K2R w KQ - 7 11" },
	{ "development", "r1b1k2r/ppppnppp/2n2q2/2b5/3NP3/2P1B3/PP3PPP/RN1QKB1R w KQkq - 0 1" },
	{ "quiet", "rnbq1rk1/pppppppp/4bn2/8/8/3B1N2/PPPPPPPP/RNBQ1RK1 w - - 0 1" },
};

static const char g_annTrainingProgressCsvHeader[] =
	"kind,game,roots,window_roots,avg_abs_error,avg_sq_error,total_avg_abs_error,total_avg_sq_error,last_abs_error,validation_samples,validation_avg_abs_error,validation_avg_sq_error,validation_last_abs_error,elapsed_ms,roots_per_sec,learn_rate\n";

static const char g_annPhaseWeightsFileMagic[] = "ANN_PHASE_WEIGHTS_V1";
static const char g_annWeightsFileMagic[] = "ANN_WEIGHTS_V1";

static int IsTrainingMoveValid(Move move)
{
	return move.From < 64 && move.To < 64 && move.MoveInfo != NotAMove;
}

static int GetAnnValidationPositionCount(void)
{
	return (int)(sizeof(g_annValidationPositions) / sizeof(g_annValidationPositions[0]));
}

static void CopyGame(Game *copy, const Game *source)
{
	if (copy == NULL || source == NULL)
		return;

	*copy = *source;
	FixPieceChain(copy);
}

static void FormatTrainingMove(Move move, char *buffer)
{
	strcpy(buffer, "0000");
	if (IsTrainingMoveValid(move))
		MoveToString(move, buffer);
}

static double GetAnnPieceMagnitude(PieceType piece)
{
	switch (piece & 7)
	{
	case PAWN:
		return 1.0 / 6.0;
	case KNIGHT:
		return 2.0 / 6.0;
	case BISHOP:
		return 3.0 / 6.0;
	case ROOK:
		return 4.0 / 6.0;
	case QUEEN:
		return 5.0 / 6.0;
	case KING:
		return 1.0;
	default:
		return 0.0;
	}
}

static int ClampAnnPhaseIndex(int phase)
{
	if (phase < 0)
		return 0;
	if (phase >= ANN_PHASE_COUNT)
		return ANN_PHASE_COUNT - 1;
	return phase;
}

static size_t GetAnnInputHiddenWeightCount(const ANN *ann)
{
	return (ann->InputCount + 1) * ann->HiddenCount;
}

static size_t GetAnnHiddenOutputWeightCount(const ANN *ann)
{
	return (ann->HiddenCount + 1) * ann->OutputCount;
}

static int WriteAnnWeightValues(FILE *file, const double *weights, size_t count)
{
	if (file == NULL || weights == NULL)
		return -1;

	for (size_t index = 0; index < count; index++)
	{
		if (fprintf(file, "%.17g\n", weights[index]) < 0)
			return -1;
	}

	return 0;
}

static int ReadAnnWeightValues(FILE *file, double *weights, size_t count)
{
	if (file == NULL || weights == NULL)
		return -1;

	for (size_t index = 0; index < count; index++)
	{
		if (fscanf(file, " %lf", &weights[index]) != 1)
			return -1;
	}

	return 0;
}

static int WriteAnnWeightsBlock(FILE *file, const ANN *ann, bool includeHeader)
{
	if (file == NULL || ann == NULL)
		return -1;

	if (includeHeader &&
		fprintf(file, "%s\n%zu %zu %zu\n", g_annWeightsFileMagic, ann->InputCount, ann->HiddenCount, ann->OutputCount) < 0)
	{
		return -1;
	}

	if (WriteAnnWeightValues(file, ann->WeightsInputHidden, GetAnnInputHiddenWeightCount(ann)) != 0)
		return -1;
	if (WriteAnnWeightValues(file, ann->WeightsHiddenOutput, GetAnnHiddenOutputWeightCount(ann)) != 0)
		return -1;

	return 0;
}

static int ReadAnnWeightsBlock(FILE *file, ANN *ann, bool headerAlreadyRead, size_t inputCount, size_t hiddenCount, size_t outputCount)
{
	char header[32];

	if (file == NULL || ann == NULL)
		return -1;

	if (!headerAlreadyRead)
	{
		if (fscanf(file, " %31s", header) != 1 ||
			strcmp(header, g_annWeightsFileMagic) != 0 ||
			fscanf(file, " %zu %zu %zu", &inputCount, &hiddenCount, &outputCount) != 3)
		{
			return -1;
		}
	}

	if (inputCount != ann->InputCount || hiddenCount != ann->HiddenCount || outputCount != ann->OutputCount)
		return -1;
	if (ReadAnnWeightValues(file, ann->WeightsInputHidden, GetAnnInputHiddenWeightCount(ann)) != 0)
		return -1;
	if (ReadAnnWeightValues(file, ann->WeightsHiddenOutput, GetAnnHiddenOutputWeightCount(ann)) != 0)
		return -1;

	return 0;
}

static void CopyAnnWeights(ANN *destination, const ANN *source)
{
	if (destination == NULL || source == NULL)
		return;

	memcpy(destination->WeightsInputHidden,
		source->WeightsInputHidden,
		GetAnnInputHiddenWeightCount(source) * sizeof(*source->WeightsInputHidden));
	memcpy(destination->WeightsHiddenOutput,
		source->WeightsHiddenOutput,
		GetAnnHiddenOutputWeightCount(source) * sizeof(*source->WeightsHiddenOutput));
}

static long long AnnTrainingNowMs(void)
{
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static void PrintTrainingUsage(const char *programName)
{
	printf("Usage: %s train-ann [--games N] [--movetime MS] [--depth N] [--hidden N] [--learn-rate X] [--adaptive-learn-rate|--fixed-learn-rate] [--learn-rate-min X] [--learn-rate-decay X] [--learn-rate-patience N] [--learn-rate-epsilon X] [--max-plies N] [--opening-plies N] [--checkpoint-games N] [--progress-interval N] [--updates-per-position N] [--validation-depth N] [--progress-csv FILE] [--seed N] [--load FILE] [--save FILE]\n", programName);
}

static void PrintTrainingProgress(const AnnTrainingProgressStats *stats, const AnnValidationStats *validation, int gameNumber, long long startMs)
{
	long long elapsedMs;
	double rootsPerSecond;
	double validationAvgAbs;
	double validationAvgSq;

	if (stats == NULL || stats->WindowSamples == 0)
		return;

	elapsedMs = AnnTrainingNowMs() - startMs;
	rootsPerSecond = elapsedMs > 0 ? ((double)stats->TotalSamples * 1000.0) / (double)elapsedMs : 0.0;
	validationAvgAbs = GetAnnValidationAverageAbsError(validation);
	validationAvgSq = GetAnnValidationAverageSquaredError(validation);

	printf("train-ann progress game %d roots %d window-roots %d avg-abs-error %.6f avg-sq-error %.6f total-avg-abs-error %.6f total-avg-sq-error %.6f last-abs-error %.6f validation-avg-abs-error %.6f validation-avg-sq-error %.6f roots-per-sec %.1f\n",
		gameNumber,
		stats->TotalSamples,
		stats->WindowSamples,
		GetAnnTrainingWindowAverageAbsError(stats),
		GetAnnTrainingWindowAverageSquaredError(stats),
		GetAnnTrainingAverageAbsError(stats),
		GetAnnTrainingAverageSquaredError(stats),
		stats->LastAbsoluteError,
		validationAvgAbs,
		validationAvgSq,
		rootsPerSecond);
}

static int OpenTrainingProgressCsv(const AnnTrainingOptions *options, FILE **csvFile)
{
	if (csvFile == NULL)
		return -1;

	*csvFile = NULL;
	if (options == NULL || options->ProgressCsvPath == NULL)
		return 0;

	*csvFile = fopen(options->ProgressCsvPath, "w");
	if (*csvFile == NULL)
	{
		fprintf(stderr, "Failed to open progress csv file %s.\n", options->ProgressCsvPath);
		return -1;
	}

	if (fputs(GetAnnTrainingProgressCsvHeader(), *csvFile) == EOF)
	{
		fprintf(stderr, "Failed to write csv header to %s.\n", options->ProgressCsvPath);
		fclose(*csvFile);
		*csvFile = NULL;
		return -1;
	}

	fflush(*csvFile);
	return 0;
}

static int WriteTrainingProgressCsv(FILE *csvFile, const char *kind, int gameNumber, long long startMs, double learnRate, const AnnTrainingProgressStats *stats, const AnnValidationStats *validation)
{
	char row[512];
	long long elapsedMs;
	double rootsPerSecond;

	if (csvFile == NULL)
		return 0;

	elapsedMs = AnnTrainingNowMs() - startMs;
	rootsPerSecond = elapsedMs > 0 ? ((double)stats->TotalSamples * 1000.0) / (double)elapsedMs : 0.0;
	if (FormatAnnTrainingProgressCsvRow(row, sizeof(row), kind, gameNumber, elapsedMs, rootsPerSecond, learnRate, stats, validation) != 0)
		return -1;

	if (fputs(row, csvFile) == EOF)
		return -1;

	fflush(csvFile);
	return 0;
}

static int ParseIntValue(const char *text, int minimumValue, int *parsed)
{
	char *end = NULL;
	long value;

	if (text == NULL || parsed == NULL)
		return -1;

	value = strtol(text, &end, 10);
	if (end == text || *end != '\0' || value < minimumValue || value > INT_MAX)
		return -1;

	*parsed = (int)value;
	return 0;
}

static int ParseDoubleValue(const char *text, double minimumValue, double *parsed)
{
	char *end = NULL;
	double value;

	if (text == NULL || parsed == NULL)
		return -1;

	value = strtod(text, &end);
	if (end == text || *end != '\0' || value < minimumValue)
		return -1;

	*parsed = value;
	return 0;
}

static int SaveTrainingWeights(const AnnPhaseCollection *collection, const AnnTrainingOptions *options, int completedGames)
{
	if (options->SavePath == NULL)
		return 0;

	if (SaveAnnPhaseWeights(collection, options->SavePath) != 0)
	{
		fprintf(stderr, "Failed to save ANN phase weights to %s after %d completed games.\n", options->SavePath, completedGames);
		return -1;
	}

	printf("train-ann checkpoint games %d saved %s\n", completedGames, options->SavePath);
	return 0;
}

static void PrepareTrainingSearch(const AnnTrainingOptions *options)
{
	SetSearchDefaults();
	g_topSearchParams.TimeControl = false;
	g_topSearchParams.EmitInfoLines = false;
	g_topSearchParams.MoveTime = options->MoveTimeMs;
	if (options->Depth > 0)
		g_topSearchParams.MaxDepth = min(options->Depth, MAX_DEPTH);
}

static int GetAnnValidationDepth(const AnnTrainingOptions *options)
{
	if (options == NULL || options->ValidationDepth <= 0)
		return ANN_TRAINING_DEFAULT_VALIDATION_DEPTH;

	return min(options->ValidationDepth, MAX_DEPTH);
}

static void PrepareValidationSearch(const AnnTrainingOptions *options)
{
	SetSearchDefaults();
	g_topSearchParams.TimeControl = false;
	g_topSearchParams.EmitInfoLines = false;
	g_topSearchParams.MoveTime = 0;
	g_topSearchParams.MaxDepth = GetAnnValidationDepth(options);
}

static int ParseTrainingArgs(int argc, char *argv[], int argStartIndex, AnnTrainingOptions *options)
{
	for (int index = argStartIndex; index < argc; index++)
	{
		char *arg = argv[index];

		if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)
		{
			PrintTrainingUsage(argv[0]);
			return 1;
		}

		if (strcmp(arg, "--adaptive-learn-rate") == 0)
		{
			options->AdaptiveLearnRate = true;
			continue;
		}

		if (strcmp(arg, "--fixed-learn-rate") == 0)
		{
			options->AdaptiveLearnRate = false;
			continue;
		}

		if (index + 1 >= argc)
		{
			fprintf(stderr, "Missing value after %s.\n", arg);
			PrintTrainingUsage(argv[0]);
			return -1;
		}

		char *value = argv[++index];
		if (strcmp(arg, "--games") == 0)
		{
			if (ParseIntValue(value, 1, &options->Games) != 0)
				goto invalid_value;
		}
		else if (strcmp(arg, "--movetime") == 0)
		{
			if (ParseIntValue(value, 0, &options->MoveTimeMs) != 0)
				goto invalid_value;
		}
		else if (strcmp(arg, "--depth") == 0)
		{
			if (ParseIntValue(value, 0, &options->Depth) != 0)
				goto invalid_value;
		}
		else if (strcmp(arg, "--hidden") == 0)
		{
			if (ParseIntValue(value, 1, &options->HiddenCount) != 0)
				goto invalid_value;
		}
		else if (strcmp(arg, "--max-plies") == 0)
		{
			if (ParseIntValue(value, 1, &options->MaxPlies) != 0)
				goto invalid_value;
		}
		else if (strcmp(arg, "--opening-plies") == 0)
		{
			if (ParseIntValue(value, 0, &options->RandomOpeningPlies) != 0)
				goto invalid_value;
		}
		else if (strcmp(arg, "--checkpoint-games") == 0)
		{
			if (ParseIntValue(value, 0, &options->CheckpointGames) != 0)
				goto invalid_value;
		}
		else if (strcmp(arg, "--progress-interval") == 0)
		{
			if (ParseIntValue(value, 0, &options->ProgressInterval) != 0)
				goto invalid_value;
		}
		else if (strcmp(arg, "--updates-per-position") == 0)
		{
			if (ParseIntValue(value, 1, &options->UpdatesPerPosition) != 0)
				goto invalid_value;
		}
		else if (strcmp(arg, "--validation-depth") == 0)
		{
			if (ParseIntValue(value, 1, &options->ValidationDepth) != 0)
				goto invalid_value;
		}
		else if (strcmp(arg, "--progress-csv") == 0)
		{
			options->ProgressCsvPath = value;
		}
		else if (strcmp(arg, "--seed") == 0)
		{
			int parsedSeed = 0;
			if (ParseIntValue(value, 0, &parsedSeed) != 0)
				goto invalid_value;
			options->Seed = (unsigned int)parsedSeed;
		}
		else if (strcmp(arg, "--learn-rate") == 0)
		{
			if (ParseDoubleValue(value, 0.0, &options->LearnRate) != 0)
				goto invalid_value;
		}
		else if (strcmp(arg, "--learn-rate-min") == 0)
		{
			if (ParseDoubleValue(value, 0.0, &options->LearnRateMin) != 0)
				goto invalid_value;
		}
		else if (strcmp(arg, "--learn-rate-decay") == 0)
		{
			if (ParseDoubleValue(value, 0.0, &options->LearnRateDecayFactor) != 0 ||
				options->LearnRateDecayFactor <= 0.0 || options->LearnRateDecayFactor >= 1.0)
			{
				goto invalid_value;
			}
		}
		else if (strcmp(arg, "--learn-rate-patience") == 0)
		{
			if (ParseIntValue(value, 1, &options->LearnRatePatience) != 0)
				goto invalid_value;
		}
		else if (strcmp(arg, "--learn-rate-epsilon") == 0)
		{
			if (ParseDoubleValue(value, 0.0, &options->LearnRateImprovementEpsilon) != 0)
				goto invalid_value;
		}
		else if (strcmp(arg, "--load") == 0)
		{
			options->LoadPath = value;
		}
		else if (strcmp(arg, "--save") == 0)
		{
			options->SavePath = value;
		}
		else
		{
			fprintf(stderr, "Unknown option: %s\n", arg);
			PrintTrainingUsage(argv[0]);
			return -1;
		}

		continue;

	invalid_value:
		fprintf(stderr, "Invalid value '%s' for %s.\n", value, arg);
		PrintTrainingUsage(argv[0]);
		return -1;
	}

	return 0;
}

void InitAnnTrainingOptions(AnnTrainingOptions *options)
{
	if (options == NULL)
		return;

	*options = (AnnTrainingOptions){0};
	options->Games = ANN_TRAINING_DEFAULT_GAMES;
	options->MoveTimeMs = ANN_TRAINING_DEFAULT_MOVETIME_MS;
	options->Depth = 0;
	options->HiddenCount = ANN_TRAINING_DEFAULT_HIDDEN_COUNT;
	options->MaxPlies = ANN_TRAINING_DEFAULT_MAX_PLIES;
	options->RandomOpeningPlies = ANN_TRAINING_DEFAULT_OPENING_PLIES;
	options->CheckpointGames = ANN_TRAINING_DEFAULT_CHECKPOINT_GAMES;
	options->ProgressInterval = ANN_TRAINING_DEFAULT_PROGRESS_INTERVAL;
	options->UpdatesPerPosition = ANN_TRAINING_DEFAULT_UPDATES_PER_POSITION;
	options->ValidationDepth = ANN_TRAINING_DEFAULT_VALIDATION_DEPTH;
	options->Seed = (unsigned int)time(NULL);
	options->LearnRate = ANN_TRAINING_DEFAULT_LEARN_RATE;
	options->AdaptiveLearnRate = ANN_TRAINING_DEFAULT_ADAPTIVE_LEARN_RATE;
	options->LearnRateMin = ANN_TRAINING_DEFAULT_LEARN_RATE_MIN;
	options->LearnRateDecayFactor = ANN_TRAINING_DEFAULT_LEARN_RATE_DECAY_FACTOR;
	options->LearnRatePatience = ANN_TRAINING_DEFAULT_LEARN_RATE_PATIENCE;
	options->LearnRateImprovementEpsilon = ANN_TRAINING_DEFAULT_LEARN_RATE_IMPROVEMENT_EPSILON;
	options->ProgressCsvPath = NULL;
	options->LoadPath = NULL;
	options->SavePath = "ann_training_weights.txt";
}

void InitAnnPhaseCollection(AnnPhaseCollection *collection)
{
	if (collection == NULL)
		return;

	*collection = (AnnPhaseCollection){0};
}

void FreeAnnPhaseCollection(AnnPhaseCollection *collection)
{
	if (collection == NULL)
		return;

	for (int phase = 0; phase < ANN_PHASE_COUNT; phase++)
	{
		if (collection->Networks[phase] != NULL)
		{
			FreeAnn(collection->Networks[phase]);
			collection->Networks[phase] = NULL;
		}
	}
	collection->HiddenCount = 0;
}

int AllocateAnnPhaseCollection(AnnPhaseCollection *collection, size_t hiddenCount)
{
	if (collection == NULL || hiddenCount == 0)
		return -1;

	FreeAnnPhaseCollection(collection);
	for (int phase = 0; phase < ANN_PHASE_COUNT; phase++)
	{
		collection->Networks[phase] = NewAnn(ANN_TRAINING_INPUT_COUNT, hiddenCount, ANN_TRAINING_OUTPUT_COUNT);
		if (collection->Networks[phase] == NULL)
		{
			FreeAnnPhaseCollection(collection);
			return -1;
		}
	}

	collection->HiddenCount = hiddenCount;
	return 0;
}

ANN *GetAnnPhaseNetwork(AnnPhaseCollection *collection, int phase)
{
	if (collection == NULL || collection->HiddenCount == 0)
		return NULL;

	return collection->Networks[ClampAnnPhaseIndex(phase)];
}

void SetAnnPhaseCollectionLearnRate(AnnPhaseCollection *collection, double learnRate)
{
	if (collection == NULL)
		return;

	for (int phase = 0; phase < ANN_PHASE_COUNT; phase++)
	{
		if (collection->Networks[phase] != NULL)
			collection->Networks[phase]->LearnRate = learnRate;
	}
}

static void MaybeAdjustAnnLearningRate(AnnPhaseCollection *collection, AnnLearningRateScheduler *scheduler, const AnnValidationStats *validation, int gameNumber, int trainedRoots)
{
	double validationAvgSq;

	if (collection == NULL || scheduler == NULL || validation == NULL)
		return;

	validationAvgSq = GetAnnValidationAverageSquaredError(validation);
	if (!UpdateAnnLearningRateScheduler(scheduler, validationAvgSq))
		return;

	SetAnnPhaseCollectionLearnRate(collection, scheduler->CurrentLearnRate);
	printf("train-ann adaptive-learn-rate game %d roots %d learn-rate %.6f validation-avg-sq-error %.6f\n",
		gameNumber,
		trainedRoots,
		scheduler->CurrentLearnRate,
		validationAvgSq);
}

int SaveAnnPhaseWeights(const AnnPhaseCollection *collection, const char *filePath)
{
	FILE *file;
	int status = -1;

	if (collection == NULL || filePath == NULL || filePath[0] == '\0' || collection->HiddenCount == 0)
		return -1;

	file = fopen(filePath, "w");
	if (file == NULL)
		return -1;

	if (fprintf(file, "%s\n%d %d %zu %d\n", g_annPhaseWeightsFileMagic, ANN_PHASE_COUNT, ANN_TRAINING_INPUT_COUNT, collection->HiddenCount, ANN_TRAINING_OUTPUT_COUNT) < 0)
		goto cleanup;

	for (int phase = 0; phase < ANN_PHASE_COUNT; phase++)
	{
		const ANN *ann = collection->Networks[phase];
		if (ann == NULL || fprintf(file, "phase %d\n", phase) < 0 || WriteAnnWeightsBlock(file, ann, false) != 0)
			goto cleanup;
	}

	status = 0;

cleanup:
	if (fclose(file) != 0)
		status = -1;
	return status;
}

int LoadAnnPhaseWeights(AnnPhaseCollection *collection, const char *filePath)
{
	FILE *file;
	AnnPhaseCollection loaded;
	char header[32];
	char phaseLabel[16];
	char trailer[2];
	size_t phaseCount = 0;
	size_t inputCount = 0;
	size_t hiddenCount = 0;
	size_t outputCount = 0;
	int status = -1;

	if (collection == NULL || filePath == NULL || filePath[0] == '\0')
		return -1;

	file = fopen(filePath, "r");
	if (file == NULL)
		return -1;

	InitAnnPhaseCollection(&loaded);
	if (fscanf(file, " %31s", header) != 1)
		goto cleanup;

	if (strcmp(header, g_annPhaseWeightsFileMagic) == 0)
	{
		if (fscanf(file, " %zu %zu %zu %zu", &phaseCount, &inputCount, &hiddenCount, &outputCount) != 4 ||
			phaseCount != ANN_PHASE_COUNT ||
			inputCount != ANN_TRAINING_INPUT_COUNT ||
			outputCount != ANN_TRAINING_OUTPUT_COUNT ||
			AllocateAnnPhaseCollection(&loaded, hiddenCount) != 0)
		{
			goto cleanup;
		}

		for (int phase = 0; phase < ANN_PHASE_COUNT; phase++)
		{
			int phaseIndex = -1;

			if (fscanf(file, " %15s %d", phaseLabel, &phaseIndex) != 2 ||
				strcmp(phaseLabel, "phase") != 0 ||
				phaseIndex != phase ||
				ReadAnnWeightsBlock(file, loaded.Networks[phase], true, inputCount, hiddenCount, outputCount) != 0)
			{
				goto cleanup;
			}
		}
	}
	else if (strcmp(header, g_annWeightsFileMagic) == 0)
	{
		if (fscanf(file, " %zu %zu %zu", &inputCount, &hiddenCount, &outputCount) != 3 ||
			inputCount != ANN_TRAINING_INPUT_COUNT ||
			outputCount != ANN_TRAINING_OUTPUT_COUNT ||
			AllocateAnnPhaseCollection(&loaded, hiddenCount) != 0 ||
			ReadAnnWeightsBlock(file, loaded.Networks[0], true, inputCount, hiddenCount, outputCount) != 0)
		{
			goto cleanup;
		}

		for (int phase = 1; phase < ANN_PHASE_COUNT; phase++)
			CopyAnnWeights(loaded.Networks[phase], loaded.Networks[0]);
	}
	else
	{
		goto cleanup;
	}

	if (fscanf(file, " %1s", trailer) != EOF)
		goto cleanup;

	FreeAnnPhaseCollection(collection);
	*collection = loaded;
	loaded = (AnnPhaseCollection){0};
	status = 0;

cleanup:
	FreeAnnPhaseCollection(&loaded);
	if (fclose(file) != 0 && status == 0)
		status = -1;
	return status;
}

void InitAnnValidationCache(AnnValidationCache *cache)
{
	if (cache == NULL)
		return;

	*cache = (AnnValidationCache){0};
}

void InitAnnTrainingProgressStats(AnnTrainingProgressStats *stats)
{
	if (stats == NULL)
		return;

	*stats = (AnnTrainingProgressStats){0};
}

void InitAnnLearningRateScheduler(AnnLearningRateScheduler *scheduler, const AnnTrainingOptions *options)
{
	double baseLearnRate;
	double minLearnRate;

	if (scheduler == NULL)
		return;

	baseLearnRate = options == NULL ? ANN_TRAINING_DEFAULT_LEARN_RATE : options->LearnRate;
	minLearnRate = options == NULL ? ANN_TRAINING_DEFAULT_LEARN_RATE_MIN : options->LearnRateMin;
	if (minLearnRate > baseLearnRate)
		minLearnRate = baseLearnRate;

	*scheduler = (AnnLearningRateScheduler){0};
	scheduler->Enabled = options == NULL ? ANN_TRAINING_DEFAULT_ADAPTIVE_LEARN_RATE : options->AdaptiveLearnRate;
	scheduler->CurrentLearnRate = baseLearnRate;
	scheduler->MinLearnRate = minLearnRate;
	scheduler->DecayFactor = options == NULL ? ANN_TRAINING_DEFAULT_LEARN_RATE_DECAY_FACTOR : options->LearnRateDecayFactor;
	scheduler->Patience = options == NULL ? ANN_TRAINING_DEFAULT_LEARN_RATE_PATIENCE : options->LearnRatePatience;
	scheduler->ImprovementEpsilon = options == NULL ? ANN_TRAINING_DEFAULT_LEARN_RATE_IMPROVEMENT_EPSILON : options->LearnRateImprovementEpsilon;
	scheduler->BestValidationSquaredError = -1.0;
}

bool UpdateAnnLearningRateScheduler(AnnLearningRateScheduler *scheduler, double validationSquaredError)
{
	double decayedLearnRate;

	if (scheduler == NULL)
		return false;

	if (scheduler->BestValidationSquaredError < 0.0 ||
		validationSquaredError + scheduler->ImprovementEpsilon < scheduler->BestValidationSquaredError)
	{
		scheduler->BestValidationSquaredError = validationSquaredError;
		scheduler->PlateauCount = 0;
		return false;
	}

	if (!scheduler->Enabled || scheduler->Patience <= 0 || scheduler->CurrentLearnRate <= scheduler->MinLearnRate)
		return false;

	scheduler->PlateauCount++;
	if (scheduler->PlateauCount < scheduler->Patience)
		return false;

	decayedLearnRate = scheduler->CurrentLearnRate * scheduler->DecayFactor;
	if (decayedLearnRate < scheduler->MinLearnRate)
		decayedLearnRate = scheduler->MinLearnRate;
	if (decayedLearnRate >= scheduler->CurrentLearnRate)
	{
		scheduler->PlateauCount = 0;
		return false;
	}

	scheduler->CurrentLearnRate = decayedLearnRate;
	scheduler->PlateauCount = 0;
	return true;
}

void AddAnnTrainingProgressSample(AnnTrainingProgressStats *stats, double prediction, double target)
{
	double difference;
	double absoluteError;
	double squaredError;

	if (stats == NULL)
		return;

	difference = target - prediction;
	absoluteError = fabs(difference);
	squaredError = difference * difference;

	stats->TotalSamples++;
	stats->WindowSamples++;
	stats->TotalAbsoluteError += absoluteError;
	stats->TotalSquaredError += squaredError;
	stats->WindowAbsoluteError += absoluteError;
	stats->WindowSquaredError += squaredError;
	stats->LastAbsoluteError = absoluteError;
	stats->LastSquaredError = squaredError;
}

void ResetAnnTrainingProgressWindow(AnnTrainingProgressStats *stats)
{
	if (stats == NULL)
		return;

	stats->WindowSamples = 0;
	stats->WindowAbsoluteError = 0.0;
	stats->WindowSquaredError = 0.0;
}

double GetAnnTrainingAverageAbsError(const AnnTrainingProgressStats *stats)
{
	if (stats == NULL || stats->TotalSamples == 0)
		return 0.0;

	return stats->TotalAbsoluteError / (double)stats->TotalSamples;
}

double GetAnnTrainingAverageSquaredError(const AnnTrainingProgressStats *stats)
{
	if (stats == NULL || stats->TotalSamples == 0)
		return 0.0;

	return stats->TotalSquaredError / (double)stats->TotalSamples;
}

double GetAnnTrainingWindowAverageAbsError(const AnnTrainingProgressStats *stats)
{
	if (stats == NULL || stats->WindowSamples == 0)
		return 0.0;

	return stats->WindowAbsoluteError / (double)stats->WindowSamples;
}

double GetAnnTrainingWindowAverageSquaredError(const AnnTrainingProgressStats *stats)
{
	if (stats == NULL || stats->WindowSamples == 0)
		return 0.0;

	return stats->WindowSquaredError / (double)stats->WindowSamples;
}

void InitAnnValidationStats(AnnValidationStats *stats)
{
	if (stats == NULL)
		return;

	*stats = (AnnValidationStats){0};
}

void AddAnnValidationSample(AnnValidationStats *stats, double prediction, double target)
{
	double difference;
	double absoluteError;
	double squaredError;

	if (stats == NULL)
		return;

	difference = target - prediction;
	absoluteError = fabs(difference);
	squaredError = difference * difference;

	stats->SampleCount++;
	stats->TotalAbsoluteError += absoluteError;
	stats->TotalSquaredError += squaredError;
	stats->LastAbsoluteError = absoluteError;
	stats->LastSquaredError = squaredError;
}

double GetAnnValidationAverageAbsError(const AnnValidationStats *stats)
{
	if (stats == NULL || stats->SampleCount == 0)
		return 0.0;

	return stats->TotalAbsoluteError / (double)stats->SampleCount;
}

double GetAnnValidationAverageSquaredError(const AnnValidationStats *stats)
{
	if (stats == NULL || stats->SampleCount == 0)
		return 0.0;

	return stats->TotalSquaredError / (double)stats->SampleCount;
}

const char *GetAnnTrainingProgressCsvHeader(void)
{
	return g_annTrainingProgressCsvHeader;
}

int FormatAnnTrainingProgressCsvRow(char *buffer, size_t bufferSize, const char *kind, int gameNumber, long long elapsedMs, double rootsPerSecond, double learnRate, const AnnTrainingProgressStats *stats, const AnnValidationStats *validation)
{
	int written;
	const char *rowKind = kind == NULL ? "progress" : kind;
	int validationSamples = validation == NULL ? 0 : validation->SampleCount;
	double validationAvgAbs = GetAnnValidationAverageAbsError(validation);
	double validationAvgSq = GetAnnValidationAverageSquaredError(validation);
	double validationLastAbs = validation == NULL ? 0.0 : validation->LastAbsoluteError;

	if (buffer == NULL || bufferSize == 0 || stats == NULL)
		return -1;

	written = snprintf(buffer,
		bufferSize,
		"%s,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%d,%.6f,%.6f,%.6f,%lld,%.1f,%.6f\n",
		rowKind,
		gameNumber,
		stats->TotalSamples,
		stats->WindowSamples,
		GetAnnTrainingWindowAverageAbsError(stats),
		GetAnnTrainingWindowAverageSquaredError(stats),
		GetAnnTrainingAverageAbsError(stats),
		GetAnnTrainingAverageSquaredError(stats),
		stats->LastAbsoluteError,
		validationSamples,
		validationAvgAbs,
		validationAvgSq,
		validationLastAbs,
		elapsedMs,
		rootsPerSecond,
		learnRate);

	return written > 0 && (size_t)written < bufferSize ? 0 : -1;
}

int GetAnnPhaseIndex(const Game *game)
{
	if (game == NULL)
		return -1;

	return ClampAnnPhaseIndex(GetGamePhase((Game *)game));
}

double EncodeAnnSquare(PieceType piece)
{
	double magnitude = GetAnnPieceMagnitude(piece);
	if (magnitude == 0.0)
		return 0.0;

	return (piece & BLACK) != 0 ? -magnitude : magnitude;
}

int EncodeAnnInputs(const Game *game, double *inputs, size_t inputCount)
{
	if (game == NULL || inputs == NULL || inputCount < ANN_TRAINING_INPUT_COUNT)
		return -1;

	for (int square = 0; square < 64; square++)
	{
		PieceType piece = game->Squares[square];
		int targetSquare = game->Side == WHITE ? square : (63 - square);
		double magnitude = GetAnnPieceMagnitude(piece);
		if (magnitude == 0.0)
		{
			inputs[targetSquare] = 0.0;
			continue;
		}

		bool isBlackPiece = (piece & BLACK) != 0;
		bool isOwnPiece = game->Side == WHITE ? !isBlackPiece : isBlackPiece;
		inputs[targetSquare] = isOwnPiece ? magnitude : -magnitude;
	}
	return 0;
}

double NormalizeAnnRelativeScore(short score)
{
	return tanh((double)score / (double)ANN_TRAINING_SCORE_SCALE);
}

double ScaleAnnPredictionToCentipawns(double value)
{
	const double limit = 0.999999;
	if (value > limit)
		value = limit;
	if (value < -limit)
		value = -limit;
	return atanh(value) * (double)ANN_TRAINING_SCORE_SCALE;
}

int PredictAnnOnPosition(ANN *ann, const Game *game, double *predictionOut)
{
	double inputs[ANN_TRAINING_INPUT_COUNT];

	if (ann == NULL || game == NULL || predictionOut == NULL)
		return -1;
	if (ann->InputCount != ANN_TRAINING_INPUT_COUNT || ann->OutputCount != ANN_TRAINING_OUTPUT_COUNT)
		return -1;
	if (EncodeAnnInputs(game, inputs, ANN_TRAINING_INPUT_COUNT) != 0)
		return -1;
	if (Compute(ann, inputs, ANN_TRAINING_INPUT_COUNT) != 0)
		return -1;

	*predictionOut = ann->Output[0];
	return 0;
}

int PredictAnnForPhase(AnnPhaseCollection *collection, const Game *game, double *predictionOut)
{
	int phase = GetAnnPhaseIndex(game);
	ANN *ann = GetAnnPhaseNetwork(collection, phase);

	if (phase < 0 || ann == NULL)
		return -1;

	return PredictAnnOnPosition(ann, game, predictionOut);
}

int ResolveAnnTrainingMove(Move desiredMove, const Move *legalMoves, int moveCount, Move *resolvedMove)
{
	if (legalMoves == NULL || resolvedMove == NULL || moveCount < 0)
		return -1;

	for (int index = 0; index < moveCount; index++)
	{
		Move legalMove = legalMoves[index];
		if (legalMove.From != desiredMove.From || legalMove.To != desiredMove.To)
			continue;

		if (desiredMove.MoveInfo >= PromotionQueen && desiredMove.MoveInfo <= PromotionKnight &&
			legalMove.MoveInfo != desiredMove.MoveInfo)
		{
			continue;
		}

		*resolvedMove = legalMove;
		return 0;
	}

	return -1;
}

int TrainAnnOnPosition(ANN *ann, const Game *game, short relativeScore, double *predictionBefore, double *targetOut)
{
	double inputs[ANN_TRAINING_INPUT_COUNT];
	double target[ANN_TRAINING_OUTPUT_COUNT];

	if (ann == NULL || game == NULL)
		return -1;
	if (ann->InputCount != ANN_TRAINING_INPUT_COUNT || ann->OutputCount != ANN_TRAINING_OUTPUT_COUNT)
		return -1;
	if (EncodeAnnInputs(game, inputs, ANN_TRAINING_INPUT_COUNT) != 0)
		return -1;

	target[0] = NormalizeAnnRelativeScore(relativeScore);
	if (Compute(ann, inputs, ANN_TRAINING_INPUT_COUNT) != 0)
		return -1;
	if (predictionBefore != NULL)
		*predictionBefore = ann->Output[0];
	if (targetOut != NULL)
		*targetOut = target[0];
	if (BackProp(ann, target, ANN_TRAINING_OUTPUT_COUNT) != 0)
		return -1;

	return 0;
}

int TrainAnnForPhase(AnnPhaseCollection *collection, const Game *game, short relativeScore, double *predictionBefore, double *targetOut)
{
	int phase = GetAnnPhaseIndex(game);
	ANN *ann = GetAnnPhaseNetwork(collection, phase);

	if (phase < 0 || ann == NULL)
		return -1;

	return TrainAnnOnPosition(ann, game, relativeScore, predictionBefore, targetOut);
}

int BuildAnnValidationCache(const AnnTrainingOptions *options, AnnValidationCache *cache)
{
	Game savedGame;
	TopSearchParams savedParams;
	bool savedOwnBook;
	bool savedStopped;
	uint savedNodes;

	if (options == NULL || cache == NULL)
		return -1;

	InitAnnValidationCache(cache);
	cache->Depth = GetAnnValidationDepth(options);
	CopyGame(&savedGame, &g_mainGame);
	savedParams = g_topSearchParams;
	savedOwnBook = OwnBook;
	savedStopped = g_Stopped;
	savedNodes = g_SearchedNodes;
	OwnBook = false;

	for (int index = 0; index < GetAnnValidationPositionCount(); index++)
	{
		double target;
		int sampleIndex = cache->SampleCount;

		if (sampleIndex >= ANN_TRAINING_VALIDATION_MAX_SAMPLES)
		{
			CopyGame(&g_mainGame, &savedGame);
			g_topSearchParams = savedParams;
			OwnBook = savedOwnBook;
			g_Stopped = savedStopped;
			g_SearchedNodes = savedNodes;
			return -1;
		}

		ReadFen(g_annValidationPositions[index].Fen);
		ClearHashTable();
		ResetDepthTimes();
		PrepareValidationSearch(options);
		Search(false);
		if (!g_topSearchParams.HasSearchScore)
		{
			CopyGame(&g_mainGame, &savedGame);
			g_topSearchParams = savedParams;
			OwnBook = savedOwnBook;
			g_Stopped = savedStopped;
			g_SearchedNodes = savedNodes;
			return -1;
		}

		cache->Samples[sampleIndex].Phase = GetAnnPhaseIndex(&g_mainGame);
		if (cache->Samples[sampleIndex].Phase < 0)
		{
			CopyGame(&g_mainGame, &savedGame);
			g_topSearchParams = savedParams;
			OwnBook = savedOwnBook;
			g_Stopped = savedStopped;
			g_SearchedNodes = savedNodes;
			return -1;
		}
		target = NormalizeAnnRelativeScore(g_topSearchParams.SearchScore);
		cache->Samples[sampleIndex].Fen = g_annValidationPositions[index].Fen;
		cache->Samples[sampleIndex].Target = target;
		cache->SampleCount++;
	}

	CopyGame(&g_mainGame, &savedGame);
	g_topSearchParams = savedParams;
	OwnBook = savedOwnBook;
	g_Stopped = savedStopped;
	g_SearchedNodes = savedNodes;
	return cache->SampleCount > 0 ? 0 : -1;
}

int EvaluateAnnValidationSet(AnnPhaseCollection *collection, const AnnValidationCache *cache, AnnValidationStats *stats)
{
	Game savedGame;
	TopSearchParams savedParams;
	bool savedOwnBook;
	bool savedStopped;
	uint savedNodes;

	if (collection == NULL || cache == NULL || stats == NULL || cache->SampleCount <= 0)
		return -1;

	InitAnnValidationStats(stats);
	CopyGame(&savedGame, &g_mainGame);
	savedParams = g_topSearchParams;
	savedOwnBook = OwnBook;
	savedStopped = g_Stopped;
	savedNodes = g_SearchedNodes;
	OwnBook = false;

	for (int index = 0; index < cache->SampleCount; index++)
	{
		ANN *ann;
		double prediction;

		ann = GetAnnPhaseNetwork(collection, cache->Samples[index].Phase);
		if (ann == NULL)
		{
			CopyGame(&g_mainGame, &savedGame);
			g_topSearchParams = savedParams;
			OwnBook = savedOwnBook;
			g_Stopped = savedStopped;
			g_SearchedNodes = savedNodes;
			return -1;
		}

		ReadFen(cache->Samples[index].Fen);
		if (PredictAnnOnPosition(ann, &g_mainGame, &prediction) != 0)
		{
			CopyGame(&g_mainGame, &savedGame);
			g_topSearchParams = savedParams;
			OwnBook = savedOwnBook;
			g_Stopped = savedStopped;
			g_SearchedNodes = savedNodes;
			return -1;
		}

		AddAnnValidationSample(stats, prediction, cache->Samples[index].Target);
	}

	CopyGame(&g_mainGame, &savedGame);
	g_topSearchParams = savedParams;
	OwnBook = savedOwnBook;
	g_Stopped = savedStopped;
	g_SearchedNodes = savedNodes;
	return 0;
}

bool ApplyRandomOpeningPlies(Game *game, int plies)
{
	Move moves[MAX_MOVES];

	if (game == NULL || plies < 0)
		return false;

	for (int ply = 0; ply < plies; ply++)
	{
		if (IsDraw(game))
			return false;

		int moveCount = ValidMovesOnThread(game, moves);
		if (moveCount == 0)
			return false;

		int chosenMove = rand() % moveCount;
		DoMove(moves[chosenMove], game);
	}

	return true;
}

int RunAnnTraining(const AnnTrainingOptions *options)
{
	AnnPhaseCollection collection;
	AnnValidationCache validationCache;
	AnnTrainingProgressStats progress;
	AnnLearningRateScheduler learnRateScheduler;
	AnnValidationStats validation;
	FILE *progressCsv = NULL;
	bool savedOwnBook = OwnBook;
	int totalTrainedRoots = 0;
	int exitCode = 0;
	long long startMs;

	if (options == NULL)
		return 1;

	srand(options->Seed);
	OwnBook = false;
	InitAnnPhaseCollection(&collection);
	InitAnnValidationCache(&validationCache);
	InitAnnTrainingProgressStats(&progress);
	InitAnnValidationStats(&validation);
	startMs = AnnTrainingNowMs();
	if (options->LoadPath != NULL)
	{
		if (LoadAnnPhaseWeights(&collection, options->LoadPath) != 0)
		{
			fprintf(stderr, "Failed to load ANN phase weights from %s.\n", options->LoadPath);
			exitCode = 1;
			goto cleanup;
		}
	}
	else if (AllocateAnnPhaseCollection(&collection, (size_t)options->HiddenCount) != 0)
	{
		fprintf(stderr, "Failed to allocate ANN with %d hidden units.\n", options->HiddenCount);
		exitCode = 1;
		goto cleanup;
	}

	InitAnnLearningRateScheduler(&learnRateScheduler, options);
	SetAnnPhaseCollectionLearnRate(&collection, learnRateScheduler.CurrentLearnRate);
	if (OpenTrainingProgressCsv(options, &progressCsv) != 0)
	{
		exitCode = 1;
		goto cleanup;
	}
	if (BuildAnnValidationCache(options, &validationCache) != 0)
	{
		fprintf(stderr, "Failed to build fixed validation targets.\n");
		exitCode = 1;
		goto cleanup;
	}

	printf("train-ann games %d movetime %d depth %d hidden %d learn-rate %.4f adaptive-learn-rate %s learn-rate-min %.4f learn-rate-decay %.4f learn-rate-patience %d learn-rate-epsilon %.4f max-plies %d opening-plies %d checkpoint-games %d progress-interval %d updates-per-position %d validation-depth %d validation-samples %d progress-csv %s seed %u save %s\n",
		options->Games,
		options->MoveTimeMs,
		options->Depth,
		(int)collection.HiddenCount,
		learnRateScheduler.CurrentLearnRate,
		learnRateScheduler.Enabled ? "on" : "off",
		learnRateScheduler.MinLearnRate,
		learnRateScheduler.DecayFactor,
		learnRateScheduler.Patience,
		learnRateScheduler.ImprovementEpsilon,
		options->MaxPlies,
		options->RandomOpeningPlies,
		options->CheckpointGames,
		options->ProgressInterval,
		options->UpdatesPerPosition,
		validationCache.Depth,
		validationCache.SampleCount,
		options->ProgressCsvPath == NULL ? "(disabled)" : options->ProgressCsvPath,
		options->Seed,
		options->SavePath == NULL ? "(disabled)" : options->SavePath);

	for (int gameIndex = 0; gameIndex < options->Games; gameIndex++)
	{
		int trainedThisGame = 0;

		StartPosition();
		ClearHashTable();
		ResetDepthTimes();
		ApplyRandomOpeningPlies(&g_mainGame, options->RandomOpeningPlies);

		for (int ply = g_mainGame.PositionHistoryLength; ply < options->MaxPlies; ply++)
		{
			Move legalMoves[MAX_MOVES];
			Move appliedMove;
			int moveCount;

			if (IsDraw(&g_mainGame))
				break;

			moveCount = ValidMoves(legalMoves);
			if (moveCount == 0)
				break;

			if (moveCount == 1)
			{
				DoMove(legalMoves[0], &g_mainGame);
				continue;
			}

			PrepareTrainingSearch(options);
			Search(false);
			if (ResolveAnnTrainingMove(g_topSearchParams.BestMove, legalMoves, moveCount, &appliedMove) != 0)
				appliedMove = legalMoves[0];

			if (g_topSearchParams.HasSearchScore)
			{
				int phaseIndex = GetAnnPhaseIndex(&g_mainGame);
				double predictionBefore = 0.0;
				double target = 0.0;
				double absoluteError;
				double targetCentipawns;
				double predictionCentipawns;
				double absoluteErrorCentipawns;
				char moveBuffer[6];

				if (phaseIndex < 0)
				{
					fprintf(stderr, "Failed to resolve phase for game %d ply %d.\n", gameIndex + 1, ply + 1);
					exitCode = 1;
					goto cleanup;
				}
				if (TrainAnnForPhase(&collection, &g_mainGame, g_topSearchParams.SearchScore, &predictionBefore, &target) != 0)
				{
					fprintf(stderr, "Failed to train on game %d ply %d.\n", gameIndex + 1, ply + 1);
					exitCode = 1;
					goto cleanup;
				}
				for (int updateIndex = 1; updateIndex < options->UpdatesPerPosition; updateIndex++)
				{
					if (TrainAnnForPhase(&collection, &g_mainGame, g_topSearchParams.SearchScore, NULL, NULL) != 0)
					{
						fprintf(stderr, "Failed to train on game %d ply %d update %d.\n", gameIndex + 1, ply + 1, updateIndex + 1);
						exitCode = 1;
						goto cleanup;
					}
				}

				AddAnnTrainingProgressSample(&progress, predictionBefore, target);
				absoluteError = progress.LastAbsoluteError;
				targetCentipawns = ScaleAnnPredictionToCentipawns(target);
				predictionCentipawns = ScaleAnnPredictionToCentipawns(predictionBefore);
				absoluteErrorCentipawns = fabs(targetCentipawns - predictionCentipawns);
				FormatTrainingMove(appliedMove, moveBuffer);
				trainedThisGame++;
				totalTrainedRoots++;
				printf("train-ann game %d ply %d phase %d score %d target %.6f %.1fcp output %.6f %.1fcp abs-error %.6f %.1fcp bestmove %s\n",
					gameIndex + 1,
					ply + 1,
					phaseIndex,
					g_topSearchParams.SearchScore,
					target,
					targetCentipawns,
					predictionBefore,
					predictionCentipawns,
					absoluteError,
					absoluteErrorCentipawns,
					moveBuffer);

				if (options->ProgressInterval > 0 && progress.WindowSamples >= options->ProgressInterval)
				{
					if (EvaluateAnnValidationSet(&collection, &validationCache, &validation) != 0)
					{
						fprintf(stderr, "Failed to evaluate validation set at game %d.\n", gameIndex + 1);
						exitCode = 1;
						goto cleanup;
					}
					PrintTrainingProgress(&progress, &validation, gameIndex + 1, startMs);
					if (WriteTrainingProgressCsv(progressCsv, "progress", gameIndex + 1, startMs, learnRateScheduler.CurrentLearnRate, &progress, &validation) != 0)
					{
						fprintf(stderr, "Failed to write progress csv row.\n");
						exitCode = 1;
						goto cleanup;
					}
					MaybeAdjustAnnLearningRate(&collection, &learnRateScheduler, &validation, gameIndex + 1, totalTrainedRoots);
					ResetAnnTrainingProgressWindow(&progress);
				}
			}

			if (!IsTrainingMoveValid(appliedMove))
				break;
			DoMove(appliedMove, &g_mainGame);
		}

		printf("train-ann game %d complete plies %d trained-roots %d\n",
			gameIndex + 1,
			g_mainGame.PositionHistoryLength,
			trainedThisGame);

		if (progress.WindowSamples > 0)
		{
			if (EvaluateAnnValidationSet(&collection, &validationCache, &validation) != 0)
			{
				fprintf(stderr, "Failed to evaluate validation set at game %d.\n", gameIndex + 1);
				exitCode = 1;
				goto cleanup;
			}
			PrintTrainingProgress(&progress, &validation, gameIndex + 1, startMs);
			if (WriteTrainingProgressCsv(progressCsv, "progress", gameIndex + 1, startMs, learnRateScheduler.CurrentLearnRate, &progress, &validation) != 0)
			{
				fprintf(stderr, "Failed to write progress csv row.\n");
				exitCode = 1;
				goto cleanup;
			}
			MaybeAdjustAnnLearningRate(&collection, &learnRateScheduler, &validation, gameIndex + 1, totalTrainedRoots);
			ResetAnnTrainingProgressWindow(&progress);
		}

		if (options->CheckpointGames > 0 && (gameIndex + 1) % options->CheckpointGames == 0)
		{
			if (SaveTrainingWeights(&collection, options, gameIndex + 1) != 0)
			{
				exitCode = 1;
				goto cleanup;
			}
		}
	}

	{
		long long elapsedMs = AnnTrainingNowMs() - startMs;
		double rootsPerSecond = elapsedMs > 0 ? ((double)progress.TotalSamples * 1000.0) / (double)elapsedMs : 0.0;
		if (EvaluateAnnValidationSet(&collection, &validationCache, &validation) != 0)
		{
			fprintf(stderr, "Failed to evaluate validation set at training completion.\n");
			exitCode = 1;
			goto cleanup;
		}
		printf("train-ann complete trained-roots %d avg-abs-error %.6f avg-sq-error %.6f last-abs-error %.6f validation-avg-abs-error %.6f validation-avg-sq-error %.6f roots-per-sec %.1f\n",
			totalTrainedRoots,
			GetAnnTrainingAverageAbsError(&progress),
			GetAnnTrainingAverageSquaredError(&progress),
			progress.LastAbsoluteError,
			GetAnnValidationAverageAbsError(&validation),
			GetAnnValidationAverageSquaredError(&validation),
			rootsPerSecond);
		if (WriteTrainingProgressCsv(progressCsv, "final", options->Games, startMs, learnRateScheduler.CurrentLearnRate, &progress, &validation) != 0)
		{
			fprintf(stderr, "Failed to write final progress csv row.\n");
			exitCode = 1;
			goto cleanup;
		}
	}
	if (SaveTrainingWeights(&collection, options, options->Games) != 0)
		exitCode = 1;

cleanup:
	OwnBook = savedOwnBook;
	StartPosition();
	if (progressCsv != NULL)
		fclose(progressCsv);
	FreeAnnPhaseCollection(&collection);
	return exitCode;
}

int RunAnnTrainingFromArgs(int argc, char *argv[], int argStartIndex)
{
	AnnTrainingOptions options;
	int parseStatus;

	InitAnnTrainingOptions(&options);
	parseStatus = ParseTrainingArgs(argc, argv, argStartIndex, &options);
	if (parseStatus != 0)
		return parseStatus > 0 ? 0 : 1;
	return RunAnnTraining(&options);
}