#pragma once

#include <stddef.h>

#include "commons.h"
#include "ANN.h"

#define ANN_TRAINING_INPUT_COUNT 64
#define ANN_TRAINING_OUTPUT_COUNT 1
#define ANN_PHASE_COUNT 25
#define ANN_TRAINING_SCORE_SCALE 400
#define ANN_TRAINING_DEFAULT_GAMES 10
#define ANN_TRAINING_DEFAULT_MOVETIME_MS 50
#define ANN_TRAINING_DEFAULT_HIDDEN_COUNT 64
#define ANN_TRAINING_DEFAULT_MAX_PLIES 200
#define ANN_TRAINING_DEFAULT_OPENING_PLIES 4
#define ANN_TRAINING_DEFAULT_CHECKPOINT_GAMES 10
#define ANN_TRAINING_DEFAULT_PROGRESS_INTERVAL 100
#define ANN_TRAINING_DEFAULT_UPDATES_PER_POSITION 4
#define ANN_TRAINING_DEFAULT_VALIDATION_DEPTH 6
#define ANN_TRAINING_DEFAULT_LEARN_RATE 0.05
#define ANN_TRAINING_DEFAULT_ADAPTIVE_LEARN_RATE true
#define ANN_TRAINING_DEFAULT_LEARN_RATE_MIN 0.005
#define ANN_TRAINING_DEFAULT_LEARN_RATE_DECAY_FACTOR 0.5
#define ANN_TRAINING_DEFAULT_LEARN_RATE_PATIENCE 4
#define ANN_TRAINING_DEFAULT_LEARN_RATE_IMPROVEMENT_EPSILON 0.001
#define ANN_TRAINING_VALIDATION_MAX_SAMPLES 16

typedef struct {
	const char *Fen;
	int Phase;
	double Target;
} AnnValidationSample;

typedef struct {
	size_t HiddenCount;
	ANN *Networks[ANN_PHASE_COUNT];
} AnnPhaseCollection;

typedef struct {
	int SampleCount;
	int Depth;
	AnnValidationSample Samples[ANN_TRAINING_VALIDATION_MAX_SAMPLES];
} AnnValidationCache;

typedef struct {
	int SampleCount;
	double TotalAbsoluteError;
	double TotalSquaredError;
	double LastAbsoluteError;
	double LastSquaredError;
} AnnValidationStats;

typedef struct {
	int TotalSamples;
	int WindowSamples;
	double TotalAbsoluteError;
	double TotalSquaredError;
	double WindowAbsoluteError;
	double WindowSquaredError;
	double LastAbsoluteError;
	double LastSquaredError;
} AnnTrainingProgressStats;

typedef struct {
	bool Enabled;
	double CurrentLearnRate;
	double MinLearnRate;
	double DecayFactor;
	int Patience;
	int PlateauCount;
	double ImprovementEpsilon;
	double BestValidationSquaredError;
} AnnLearningRateScheduler;

typedef struct {
	int Games;
	int MoveTimeMs;
	int Depth;
	int HiddenCount;
	int MaxPlies;
	int RandomOpeningPlies;
	int CheckpointGames;
	int ProgressInterval;
	int UpdatesPerPosition;
	int ValidationDepth;
	unsigned int Seed;
	double LearnRate;
	bool AdaptiveLearnRate;
	double LearnRateMin;
	double LearnRateDecayFactor;
	int LearnRatePatience;
	double LearnRateImprovementEpsilon;
	const char *ProgressCsvPath;
	const char *LoadPath;
	const char *SavePath;
} AnnTrainingOptions;

void InitAnnTrainingOptions(AnnTrainingOptions *options);
void InitAnnPhaseCollection(AnnPhaseCollection *collection);
void FreeAnnPhaseCollection(AnnPhaseCollection *collection);
int AllocateAnnPhaseCollection(AnnPhaseCollection *collection, size_t hiddenCount);
ANN *GetAnnPhaseNetwork(AnnPhaseCollection *collection, int phase);
void SetAnnPhaseCollectionLearnRate(AnnPhaseCollection *collection, double learnRate);
int SaveAnnPhaseWeights(const AnnPhaseCollection *collection, const char *filePath);
int LoadAnnPhaseWeights(AnnPhaseCollection *collection, const char *filePath);
void InitAnnValidationCache(AnnValidationCache *cache);
void InitAnnTrainingProgressStats(AnnTrainingProgressStats *stats);
void InitAnnLearningRateScheduler(AnnLearningRateScheduler *scheduler, const AnnTrainingOptions *options);
bool UpdateAnnLearningRateScheduler(AnnLearningRateScheduler *scheduler, double validationSquaredError);
void AddAnnTrainingProgressSample(AnnTrainingProgressStats *stats, double prediction, double target);
void ResetAnnTrainingProgressWindow(AnnTrainingProgressStats *stats);
double GetAnnTrainingAverageAbsError(const AnnTrainingProgressStats *stats);
double GetAnnTrainingAverageSquaredError(const AnnTrainingProgressStats *stats);
double GetAnnTrainingWindowAverageAbsError(const AnnTrainingProgressStats *stats);
double GetAnnTrainingWindowAverageSquaredError(const AnnTrainingProgressStats *stats);
void InitAnnValidationStats(AnnValidationStats *stats);
void AddAnnValidationSample(AnnValidationStats *stats, double prediction, double target);
double GetAnnValidationAverageAbsError(const AnnValidationStats *stats);
double GetAnnValidationAverageSquaredError(const AnnValidationStats *stats);
const char *GetAnnTrainingProgressCsvHeader(void);
int FormatAnnTrainingProgressCsvRow(char *buffer, size_t bufferSize, const char *kind, int gameNumber, long long elapsedMs, double rootsPerSecond, double learnRate, const AnnTrainingProgressStats *stats, const AnnValidationStats *validation);
double EncodeAnnSquare(PieceType piece);
int EncodeAnnInputs(const Game *game, double *inputs, size_t inputCount);
double NormalizeAnnRelativeScore(short score);
double ScaleAnnPredictionToCentipawns(double value);
int GetAnnPhaseIndex(const Game *game);
int PredictAnnOnPosition(ANN *ann, const Game *game, double *predictionOut);
int PredictAnnForPhase(AnnPhaseCollection *collection, const Game *game, double *predictionOut);
int ResolveAnnTrainingMove(Move desiredMove, const Move *legalMoves, int moveCount, Move *resolvedMove);
int TrainAnnOnPosition(ANN *ann, const Game *game, short relativeScore, double *predictionBefore, double *targetOut);
int TrainAnnForPhase(AnnPhaseCollection *collection, const Game *game, short relativeScore, double *predictionBefore, double *targetOut);
bool ApplyRandomOpeningPlies(Game *game, int plies);
int BuildAnnValidationCache(const AnnTrainingOptions *options, AnnValidationCache *cache);
int EvaluateAnnValidationSet(AnnPhaseCollection *collection, const AnnValidationCache *cache, AnnValidationStats *stats);
int RunAnnTraining(const AnnTrainingOptions *options);
int RunAnnTrainingFromArgs(int argc, char *argv[], int argStartIndex);