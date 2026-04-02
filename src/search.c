#include <stdlib.h>
#include "search.h"
#include "bitboards.h"
#include "position.h"
#include "evaluation.h"
#include "moves.h"
#include "hashTable.h"
#include "timeControl.h"
#include "book.h"
#include "countermoves.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

uint g_SearchedNodes = 0;
bool g_Stopped = 0;
TopSearchParams g_topSearchParams = {0};

static PlatformThread g_timeLimitThread;
static bool g_hasTimeLimitThread = false;
static PlatformThread g_searchThread;
static bool g_hasSearchThread = false;
static bool g_emitBestMove = false;

static Move InvalidMove()
{
	Move move;
	move.From = 255;
	move.To = 255;
	move.MoveInfo = NotAMove;
	move.PieceIdx = 255;
	move.Score = 0;
	return move;
}

static bool IsMoveValid(Move move)
{
	return move.From < 64 && move.To < 64 && move.MoveInfo != NotAMove;
}

static MoveCoordinates CoordinatesFromMove(Move move)
{
	MoveCoordinates coords;
	if (IsMoveValid(move))
	{
		coords.From = move.From;
		coords.To = move.To;
	}
	else
	{
		coords.From = 255;
		coords.To = 255;
	}
	return coords;
}

static Move PlainMoveFromCoordinates(MoveCoordinates coords)
{
	Move move = InvalidMove();
	move.From = coords.From;
	move.To = coords.To;
	move.MoveInfo = PlainMove;
	return move;
}

#define HASH_MOVE_BONUS 10000
#define PRIMARY_KILLER_BONUS 900
#define SECONDARY_KILLER_BONUS 700
#define COUNTERMOVE_BONUS 500
#define KILLER_MOVE_SLOTS 2

static bool SameMoveCoordinates(Move left, Move right)
{
	return left.From == right.From && left.To == right.To;
}

static void ApplyMoveOrderingBonus(Move *move, Side side, short bonus)
{
	move->Score = (short)(move->Score + (side == BLACK ? bonus : (short)-bonus));
}

static bool IsQuietMove(const Game *game, Move move)
{
	if (!IsMoveValid(move))
		return false;

	if ((move.MoveInfo >= PromotionQueen && move.MoveInfo <= PromotionKnight) ||
		move.MoveInfo == SoonPromoting ||
		move.MoveInfo == EnPassantCapture)
	{
		return false;
	}

	return game->Squares[move.To] == NOPIECE;
}

static void EmitBestMove(Move move)
{
	if (IsMoveValid(move))
	{
		char sMove[6];
		MoveToString(move, sMove);
		printf("bestmove %s\n", sMove);
	}
	else
	{
		printf("bestmove 0000\n");
	}
	fflush(stdout);
}

static void JoinActiveSearchThreads()
{
	if (g_hasSearchThread)
	{
		g_Stopped = true;
		PlatformJoinThread(g_searchThread);
		g_hasSearchThread = false;
	}
	if (g_hasTimeLimitThread)
	{
		g_Stopped = true;
		PlatformJoinThread(g_timeLimitThread);
		g_hasTimeLimitThread = false;
	}
}

void StopSearch()
{
	g_Stopped = true;
	JoinActiveSearchThreads();
}

#define MAX_R 4
#define MIN_R 3

static uchar lmr_matrix[MAX_DEPTH + 1][100] = {0};

static long long NowMs()
{
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

void InitLmr()
{
	for (int depth = 0; depth <= MAX_DEPTH; depth++)
	{
		for (int moveNo = 0; moveNo < 100; moveNo++)
		{
			if (depth > 7 && moveNo > 10)
				lmr_matrix[depth][moveNo] = 3;
			else if (depth > 3 && moveNo > 7)
				lmr_matrix[depth][moveNo] = 2;
			else
				lmr_matrix[depth][moveNo] = 1;

			// printf("depth: %d   moveNo: %d   lmr: %d\n", depth, moveNo, lmr_matrix[depth][moveNo]);
		}
	}
}

uchar GetLmrReduction(int depth, int moveNo)
{
	if (depth < 0)
		depth = 0;
	else if (depth > MAX_DEPTH)
		depth = MAX_DEPTH;

	if (moveNo < 0)
		moveNo = 0;
	else if (moveNo > 99)
		moveNo = 99;

	return lmr_matrix[depth][moveNo];
}

void SetSearchDefaults()
{
	g_topSearchParams = (TopSearchParams){0};
	g_topSearchParams.MaxDepth = MAX_DEPTH;
	g_topSearchParams.BestMove = InvalidMove();
}

static void MoveToTop(Move move, Move *list, int length, Side side)
{
	for (int i = 0; i < length; i++)
	{
		if (SameMoveCoordinates(move, list[i]) && i > 0)
		{
			ApplyMoveOrderingBonus(&list[i], side, HASH_MOVE_BONUS);
			return;
		}
	}
}

static void PickNextMove(Side side, int moveNum, Move *moves, int moveCount)
{
	int bestNum = moveNum;
	short bestScore = moves[moveNum].Score;

	for (int index = moveNum + 1; index < moveCount; ++index)
	{
		bool isBetterMove = side == BLACK ? moves[index].Score > bestScore : moves[index].Score < bestScore;
		if (isBetterMove)
		{
			bestScore = moves[index].Score;
			bestNum = index;
		}
	}

	if (bestNum != moveNum)
	{
		Move temp = moves[moveNum];
		moves[moveNum] = moves[bestNum];
		moves[bestNum] = temp;
	}
}

static void MoveCounterMoveToTop(Game *game, Move previousMove, Move *moveList, int moveListLength)
{
	if (!IsMoveValid(previousMove))
		return;

	for (int i = 0; i < moveListLength; i++)
	{
		if (!IsQuietMove(game, moveList[i]))
			continue;

		if (IsCounterMove(moveList[i], previousMove))
		{
			ApplyMoveOrderingBonus(&moveList[i], game->Side, COUNTERMOVE_BONUS);
			break;
		}
	}
}

#define MAX_QSEARCH_DEPTH 64
#define THREAD_MOVE_BUFFER_PLY MAX_QSEARCH_DEPTH

#if defined(_MSC_VER)
#define CM_THREAD_LOCAL __declspec(thread)
#else
#define CM_THREAD_LOCAL _Thread_local
#endif

static CM_THREAD_LOCAL Move g_threadMoveBuffer[THREAD_MOVE_BUFFER_PLY][MAX_MOVES];
static CM_THREAD_LOCAL Move g_threadKillerMoves[THREAD_MOVE_BUFFER_PLY][KILLER_MOVE_SLOTS];

static Move *GetLocalMoveList(int ply, Move *fallbackMoves)
{
	if (ply >= 0 && ply < THREAD_MOVE_BUFFER_PLY)
		return g_threadMoveBuffer[ply];
	return fallbackMoves;
}

static Move *CopyMovesToLocalBuffer(const Game *game, int ply, int moveCount, Move *fallbackMoves)
{
	Move *localMoves = GetLocalMoveList(ply, fallbackMoves);
	memcpy(localMoves, game->MovesBuffer, moveCount * sizeof(Move));
	return localMoves;
}

static void ResetThreadKillerMoves()
{
	Move invalid = InvalidMove();
	for (int ply = 0; ply < THREAD_MOVE_BUFFER_PLY; ply++)
	{
		for (int slot = 0; slot < KILLER_MOVE_SLOTS; slot++)
		{
			g_threadKillerMoves[ply][slot] = invalid;
		}
	}
}

static void AddKillerMove(Move move, int ply)
{
	if (ply < 0 || ply >= THREAD_MOVE_BUFFER_PLY || !IsMoveValid(move))
		return;

	Move *killers = g_threadKillerMoves[ply];
	if (SameMoveCoordinates(killers[0], move))
		return;
	if (SameMoveCoordinates(killers[1], move))
	{
		Move temp = killers[0];
		killers[0] = killers[1];
		killers[1] = temp;
		return;
	}

	killers[1] = killers[0];
	killers[0] = move;
}

static void MoveKillersToTop(Game *game, Move *moveList, int moveListLength, int ply)
{
	if (ply < 0 || ply >= THREAD_MOVE_BUFFER_PLY)
		return;

	Move *killers = g_threadKillerMoves[ply];
	for (int i = 0; i < moveListLength; i++)
	{
		if (!IsQuietMove(game, moveList[i]))
			continue;

		if (SameMoveCoordinates(moveList[i], killers[0]))
		{
			ApplyMoveOrderingBonus(&moveList[i], game->Side, PRIMARY_KILLER_BONUS);
		}
		else if (SameMoveCoordinates(moveList[i], killers[1]))
		{
			ApplyMoveOrderingBonus(&moveList[i], game->Side, SECONDARY_KILLER_BONUS);
		}
	}
}

static short EvalForSide(Game *game)
{
	short eval = GetEval(game); // GetEval is black-centric.
	return game->Side == BLACK ? eval : (short)-eval;
}

#define QSEARCH_DELTA_MARGIN 100

static short GetQsearchPieceValue(PieceType piece)
{
	switch (piece & 7)
	{
	case PAWN:
		return MATERIAL_P;
	case KNIGHT:
		return MATERIAL_N;
	case BISHOP:
		return MATERIAL_B;
	case ROOK:
		return MATERIAL_R;
	case QUEEN:
		return MATERIAL_Q;
	default:
		return 0;
	}
}

static short GetQsearchCapturedPieceValue(Move move, const Game *game)
{
	PieceType capturedPiece = move.MoveInfo == EnPassantCapture ? (PAWN | (game->Side ^ 24)) : game->Squares[move.To];
	return GetQsearchPieceValue(capturedPiece);
}

static bool ShouldDeltaPruneQsearch(const Game *game, Move move, short standPat, short alpha)
{
	if (alpha >= 7000 || standPat <= -7000)
		return false;

	if (move.MoveInfo >= PromotionQueen && move.MoveInfo <= PromotionKnight)
		return false;

	short optimisticGain = GetQsearchCapturedPieceValue(move, game);
	if (optimisticGain <= 0)
		return false;

	return standPat + optimisticGain + QSEARCH_DELTA_MARGIN <= alpha;
}

static bool GivesCheckAfterMove(const Game *game)
{
	return SquareAttacked(game->KingSquares[game->Side01], game->Side ^ 24, (Game *)game);
}

static bool ShouldSkipBadQsearchCapture(const Game *game, Move move, PieceType movingPiece, short capturedValue)
{
	if (capturedValue <= 0)
		return false;

	if (move.MoveInfo >= PromotionQueen && move.MoveInfo <= PromotionKnight)
		return false;

	if (GivesCheckAfterMove(game))
		return false;

	short movingValue = GetQsearchPieceValue(movingPiece);
	if (movingValue <= capturedValue)
		return false;

	if (!SquareAttacked(move.To, game->Side, (Game *)game))
		return false;

	if (SquareAttacked(move.To, game->Side ^ 24, (Game *)game))
		return false;

	return true;
}

static bool TryDoLegalMove(Game *game, const LegalMoveContext *legalCtx, Move move, Undos *undos)
{
	FastMoveLegality legality = ClassifyMoveLegality(move, game, legalCtx);
	if (legality == FastMoveIllegal)
		return false;

	*undos = DoMove(move, game);
	if (legality == FastMoveNeedsFullCheck)
	{
		int kingSquare = game->KingSquares[!game->Side01];
		if (SquareAttacked(kingSquare, game->Side, game))
		{
			UndoMove(game, move, *undos);
			return false;
		}
	}

	return true;
}

short QuietSearch(short alpha, short beta, Game *game, int deep_in)
{
	if (g_Stopped)
		return 0;

	g_SearchedNodes++;
	if (deep_in >= MAX_QSEARCH_DEPTH)
		return EvalForSide(game);

	if (IsDraw(game))
		return 0;

	bool incheck = SquareAttacked(game->KingSquares[game->Side01], game->Side ^ 24, game);
	short standPat = 0;
	short score = 0;
	if (!incheck)
	{
		standPat = EvalForSide(game); // side-to-move perspective.
		score = standPat;
		if (score >= beta)
			return beta;
		if (score > alpha)
			alpha = score;

		CreateCaptureMoves(game);
	}
	else
	{
		CreateMoves(game);
	}

	int moveCount = game->MovesBufferLength;
	if (moveCount == 0)
		return incheck ? (short)(-8000 + deep_in) : alpha;
	LegalMoveContext legalCtx;
	BuildLegalMoveContext(game, &legalCtx);

	Move fallbackMoves[MAX_MOVES];
	Move *localMoves = CopyMovesToLocalBuffer(game, deep_in, moveCount, fallbackMoves);
	// MoveKillersToTop(game, localMoves, moveCount);
	int legalCount = 0;

	for (int i = 0; i < moveCount; i++)
	{
		if (g_Stopped)
			return alpha;

		PickNextMove(game->Side, i, localMoves, moveCount);

		Move childMove = localMoves[i];
		PieceType movingPiece = game->Squares[childMove.From];
		short capturedValue = GetQsearchCapturedPieceValue(childMove, game);
		if (!incheck && ShouldDeltaPruneQsearch(game, childMove, standPat, alpha))
			continue;

		Undos undos;
		if (!TryDoLegalMove(game, &legalCtx, childMove, &undos))
			continue;

		if (!incheck && ShouldSkipBadQsearchCapture(game, childMove, movingPiece, capturedValue))
		{
			UndoMove(game, childMove, undos);
			continue;
		}
		legalCount++;

		score = (short)-QuietSearch((short)-beta, (short)-alpha, game, deep_in + 1);
		UndoMove(game, childMove, undos);

		if (g_Stopped)
			return alpha;

		if (score >= beta)
			return beta;
		if (score > alpha)
			alpha = score;
	}

	if (legalCount == 0)
		return incheck ? (short)(-8000 + deep_in) : alpha;

	return alpha;
}

static bool IsReductionOk(Move move, Undos undos)
{
	return undos.CaptIndex == -1 && // no capture
		   move.MoveInfo != PromotionQueen &&
		   move.MoveInfo != SoonPromoting;
}

static bool IsPureKingAndPawnEnding(const Game *game)
{
	const AllPieceBitboards *bb = &game->Bitboards;
	return bb->Knights.AllKnights == 0ULL &&
		   bb->Bishops.AllBishops == 0ULL &&
		   bb->Rooks.AllRooks == 0ULL &&
		   bb->Queens.AllQueens == 0ULL;
}

static bool IsAdvancedPawnPushInPawnEnding(const Game *game, Move move)
{
	if (!IsPureKingAndPawnEnding(game))
		return false;

	if ((game->Squares[move.From] & 7) != PAWN)
		return false;

	int toRank = move.To >> 3;
	return game->Side01 == 0 ? toRank >= 5 : toRank <= 2;
}

short RecursiveSearch(short alpha, short beta, uchar depth, Game *game, bool doNull, Move prevMove, int deep_in, bool incheck)
{
	if (g_Stopped)
		return 0; // should not be used;

	g_SearchedNodes++;

	if (depth <= 0)
	{
		if (incheck)
			depth = 1;
		else
			return QuietSearch(alpha, beta, game, deep_in);
	}

	if (IsDraw(game))
		return 0;

	// Probe hash
	short score = 0;
	Move pvMove;
	pvMove.MoveInfo = NotAMove;
	if (GetScoreFromHash(game->Hash, depth, &score, &pvMove, alpha, beta))
	{
		return score;
	}

	// NULL move check

	if (doNull && !incheck && depth > 3 && !IsPureKingAndPawnEnding(game))
	{
		GameState prevState = game->State;
		uchar r = depth > 6 ? MAX_R : MIN_R;
		U64 prevHash = game->Hash;
		bool nullMovePushedHistory = DoNullMove(game);
		bool checkedAfterNull = SquareAttacked(game->KingSquares[game->Side01], game->Side ^ 24, game);
		short nullScore = (short)-RecursiveSearch((short)-beta, (short)(-beta + 1), depth - r, game, false, prevMove, deep_in + 1, checkedAfterNull);
		UndoNullMove(prevState, game, prevHash, nullMovePushedHistory);
		if (nullScore >= beta && nullScore > -8000 && nullScore < 8000)
			return beta;
	}

	// Move generation
	CreateMoves(game);
	uchar moveCount = game->MovesBufferLength;
	if (moveCount == 0)
	{
		if (incheck)
			return (short)(-8000 + deep_in);
		return 0;
	}
	LegalMoveContext legalCtx;
	BuildLegalMoveContext(game, &legalCtx);

	Move fallbackMoves[MAX_MOVES];
	Move *localMoves = CopyMovesToLocalBuffer(game, deep_in, moveCount, fallbackMoves);

	if (pvMove.MoveInfo != NotAMove)
	{
		MoveToTop(pvMove, localMoves, moveCount, game->Side);
	}
	if (deep_in > 0)
	{
		MoveCounterMoveToTop(game, prevMove, localMoves, moveCount);
		MoveKillersToTop(game, localMoves, moveCount, deep_in);
	}

	// Not reducing for the first number of moves of each depth.
	const uchar fullDepthMoves = 10;
	// Not reducing when depth is or lower
	const uchar reductionLimit = 3;

	// alpha beta pruning
	short bestScore = MIN_SCORE;
	uchar legalCount = 0;
	short oldAlpha = alpha;
	Move bestMove = localMoves[0];
	for (int i = 0; i < moveCount; i++)
	{
		PickNextMove(game->Side, i, localMoves, moveCount);

		Move childMove = localMoves[i];
		Undos undos;
		if (!TryDoLegalMove(game, &legalCtx, childMove, &undos))
			continue;
		legalCount++;

		// extensions
		uchar extension = 0;
		bool pawnRacePush = IsAdvancedPawnPushInPawnEnding(game, childMove);
		bool checked = SquareAttacked(game->KingSquares[game->Side01], game->Side ^ 24, game);
		if (checked || childMove.MoveInfo == SoonPromoting || pawnRacePush)
			extension = 1;

		uchar lmrRed = GetLmrReduction(depth, i);
		// Late Move Reduction, full depth for the first moves, and interesting moves.
		if (i >= fullDepthMoves && depth >= reductionLimit && extension == 0 && IsReductionOk(childMove, undos))
			score = (short)-RecursiveSearch((short)(-alpha - 1), (short)-alpha, depth - lmrRed, game, true, childMove, deep_in + 1, checked);
		else
			score = (short)(alpha + 1); // Hack to ensure that full-depth is done.

		if (score > alpha)
		{
			// surprisingly good, re-search at full depth.
			score = (short)-RecursiveSearch((short)-beta, (short)-alpha, depth - 1 + extension, game, true, childMove, deep_in + 1, checked);
		}

		UndoMove(game, childMove, undos);

		if (g_Stopped)
			return alpha;

		if (score > bestScore)
		{
			bestScore = score;
			bestMove = childMove;
		}

		if (score > alpha)
		{
			alpha = score;
			if (alpha >= beta)
			{
				AddHashScore(game->Hash, beta, depth, BEST_WHITE, bestMove);
				if (deep_in > 0 && IsQuietMove(game, childMove))
				{
					AddCounterMove(childMove, prevMove);
					AddKillerMove(childMove, deep_in);
				}
				return beta;
			}
		}
	}

	if (legalCount == 0)
	{
		if (incheck)
			return -8000 + deep_in; // mate
		else
			return 0; // stale mate
	}

	if (g_Stopped)
		return alpha;

	if (alpha != oldAlpha)
		AddHashScore(game->Hash, alpha, depth, EXACT, bestMove);
	else
		AddHashScore(game->Hash, alpha, depth, BEST_BLACK, bestMove);

	return alpha;
}

static void CopyMainGame(Game *copy)
{

	copy->Side = g_mainGame.Side;
	copy->Side01 = g_mainGame.Side01;
	copy->State = g_mainGame.State;
	copy->Hash = g_mainGame.Hash;
	copy->PositionHistoryLength = g_mainGame.PositionHistoryLength;
	copy->MovesBufferLength = g_mainGame.MovesBufferLength;
	copy->FiftyMoveRuleCount = g_mainGame.FiftyMoveRuleCount;
	copy->KingSquares[0] = g_mainGame.KingSquares[0];
	copy->KingSquares[1] = g_mainGame.KingSquares[1];
	copy->Material[0] = g_mainGame.Material[0];
	copy->Material[1] = g_mainGame.Material[1];

	memcpy(copy->MovesBuffer, g_mainGame.MovesBuffer, g_mainGame.MovesBufferLength * sizeof(copy->MovesBuffer[0]));
	memcpy(copy->Squares, g_mainGame.Squares, sizeof(copy->Squares));
	copy->Bitboards = g_mainGame.Bitboards;
	memcpy(copy->PositionHistory, g_mainGame.PositionHistory, g_mainGame.PositionHistoryLength * sizeof(copy->PositionHistory[0]));
	memcpy(copy->Pieces, g_mainGame.Pieces, sizeof(copy->Pieces));
	FixPieceChain(copy); // Pieces could not point to their game copy pieces.

	// memset(copy->KillerMoves, 0, 2 * 31 * sizeof(Move));
}

static Move GetFirstValidMove(Game *game, bool onThread)
{
	Move moves[MAX_MOVES];
	int moveCount = onThread ? ValidMovesOnThread(game, moves) : ValidMoves(moves);
	if (moveCount > 0)
		return moves[0];
	return InvalidMove();
}

void PrintBestLine(Move move, int depth, float ellapsed)
{
	char buffer[1800];
	char *pv = buffer;
	char sMove[6];
	MoveToString(move, sMove);
	strcpy(pv, sMove);
	pv += strlen(sMove);
	strcpy(pv, " ");
	pv++;
	Game *game = &g_mainGame;
	PlayerMove bestPlayerMove = MakePlayerMoveOnThread(game, sMove);
	PlayerMove moves[300];
	int movesCount = 0;
	while (movesCount < (depth + 5)) // hack: shows moves found when extending search (checks), but not going into cycles.
	{
		Move bestMove;
		if (!GetBestMoveFromHash(game->Hash, &bestMove))
			break;

		char sMove[6];
		MoveToString(bestMove, sMove);

		PlayerMove plMove = MakePlayerMoveOnThread(game, sMove);
		if (plMove.Invalid)
			break;
		moves[movesCount++] = plMove;
		strcpy(pv, sMove);
		pv += strlen(sMove);
		strcpy(pv, " ");
		pv++;
	}
	strcpy(pv, "\0");

	for (int i = movesCount - 1; i >= 0; i--)
		UnMakePlayerMoveOnThread(game, moves[i]);
	UnMakePlayerMoveOnThread(game, bestPlayerMove);
	float safeElapsed = ellapsed > 0.0001f ? ellapsed : 0.0001f;
	int nps = (int)((float)g_SearchedNodes / safeElapsed);
	int time = ellapsed * 1000;
	short score = move.Score;

	printf("info score ");
	if (abs(score) > 7000)
	{
		bool meMated = score < 0;
		int mateIn = (8000 - abs(score)) / 2 + 1;
		if (meMated)
			mateIn = -mateIn;
		printf("mate %d ", mateIn);
	}
	else
	{
		printf("cp %d ", score);
	}

	printf("depth %d nodes %d time %d nps %d hashfull %d pv %s\n", depth, g_SearchedNodes, time, nps, HashFull(), buffer);
	fflush(stdout);
}

// Background thread that sets Stopped flag after specified time in ms.
PlatformThreadReturn PLATFORM_THREAD_CALL TimeLimitWatch(void *args)
{
	(void)args;
	int ms = g_topSearchParams.MoveTime;
	long long start = NowMs();
	long long now = start;
	while (!g_Stopped)
	{
		PlatformSleepMs(100);
		now = NowMs();

		if (now - start > ms)
		{
			break;
		}
	}

	g_Stopped = true;
	return PLATFORM_THREAD_RETURN_VALUE;
}

PlatformThreadReturn PLATFORM_THREAD_CALL IterativeSearch(void *v)
{
	(void)v;
	ResetThreadKillerMoves();
	Game game;
	Game *pGame = &game;
	CopyMainGame(pGame);
	long long start = NowMs();
	Move bestMove = InvalidMove();

	Move noMove = InvalidMove();
	short lastScore = 0;
	bool hasLastScore = false;

	for (int depth = 1; depth <= g_topSearchParams.MaxDepth; depth++)
	{
		long long depStart = NowMs();
		bool incheck = SquareAttacked(pGame->KingSquares[pGame->Side01], pGame->Side ^ 24, pGame);

		short score;
		if (hasLastScore && depth >= 4)
		{
			short delta = 40;
			short aspAlpha = (short)(lastScore - delta);
			short aspBeta = (short)(lastScore + delta);
			if (aspAlpha < MIN_SCORE)
				aspAlpha = MIN_SCORE;
			if (aspBeta > MAX_SCORE)
				aspBeta = MAX_SCORE;

			while (true)
			{
				score = RecursiveSearch(aspAlpha, aspBeta, depth, pGame, true, noMove, 0, incheck);
				if (g_Stopped)
					break;

				if (score <= aspAlpha)
				{
					delta = (short)(delta * 2);
					aspAlpha = (short)(lastScore - delta);
					if (aspAlpha < MIN_SCORE)
						aspAlpha = MIN_SCORE;
				}
				else if (score >= aspBeta)
				{
					delta = (short)(delta * 2);
					aspBeta = (short)(lastScore + delta);
					if (aspBeta > MAX_SCORE)
						aspBeta = MAX_SCORE;
				}
				else
				{
					break;
				}

				if (aspAlpha == MIN_SCORE && aspBeta == MAX_SCORE)
				{
					score = RecursiveSearch(MIN_SCORE, MAX_SCORE, depth, pGame, true, noMove, 0, incheck);
					break;
				}
			}
		}
		else
		{
			score = RecursiveSearch(MIN_SCORE, MAX_SCORE, depth, pGame, true, noMove, 0, incheck);
		}
		if (g_Stopped)
			break;
		lastScore = score;
		hasLastScore = true;

		float ellapsed = (float)(NowMs() - start) / 1000.0f;
		if (GetBestMoveFromHash(pGame->Hash, &bestMove))
		{
			bestMove.Score = score;
			if (depth > 7)
				PrintBestLine(bestMove, depth, ellapsed);
		}
		g_topSearchParams.BestMove = bestMove;

		if (abs(score) > 7000)
		{
			g_Stopped = true;
			break; // A check mate is found, no need to search further.
		}

		// At the end of each depth, checks if it is smart to search deeper.
		// todo: if score has gone down, is it worth spending some more time?
		int depthTime = (int)(NowMs() - depStart);
		int moveNo = pGame->PositionHistoryLength;
		RegisterDepthTime(moveNo, depth, depthTime);
		RegisterIterationResult(moveNo, depth, bestMove, score);
		if (g_topSearchParams.TimeControl && !SearchDeeper(depth, moveNo, (int)(ellapsed * 1000), pGame->Side))
		{
			g_Stopped = true;
			break;
		}
	}

	if (!IsMoveValid(bestMove))
		bestMove = GetFirstValidMove(pGame, true);

	if (g_emitBestMove)
		EmitBestMove(bestMove);
	g_topSearchParams.BestMove = bestMove;
	g_Stopped = true;
	return PLATFORM_THREAD_RETURN_VALUE;
}

// Starting point of a search for best move.
// Continues until time millis is reached or depth is reached.
// When async is set the result is printed to stdout. Not returned.
MoveCoordinates Search(bool async)
{
	JoinActiveSearchThreads();
	SyncGameBitboards(&g_mainGame);
	g_emitBestMove = async;
	g_topSearchParams.BestMove = InvalidMove();

	// Sometimes there is only one valid move. Why spend time searching?
	Move moves[MAX_MOVES];
	int count = ValidMoves(moves);
	if (count == 1)
	{
		g_topSearchParams.BestMove = moves[0];
		if (g_emitBestMove)
			EmitBestMove(moves[0]);
		return CoordinatesFromMove(moves[0]);
	}

	// book moves are just to add variation on tournament games.
	if (g_mainGame.PositionHistoryLength < 6)
	{
		MoveCoordinates bookMove = RandomBookMove(&g_mainGame);
		if (bookMove.From != 255)
		{
			Move bookAsMove = PlainMoveFromCoordinates(bookMove);
			g_topSearchParams.BestMove = bookAsMove;
			if (g_emitBestMove)
				EmitBestMove(bookAsMove);
			return bookMove;
		}
	}

	ClearCounterMoves();

	g_Stopped = false;
	g_SearchedNodes = 0;

	g_hasTimeLimitThread = false;
	if (g_topSearchParams.MoveTime > 0)
	{
		g_hasTimeLimitThread = PlatformCreateThread(&g_timeLimitThread, TimeLimitWatch, NULL);
	}

	g_hasSearchThread = PlatformCreateThread(&g_searchThread, IterativeSearch, NULL);
	if (!g_hasSearchThread)
	{
		IterativeSearch(NULL);
	}

	if (!async)
	{
		if (g_hasSearchThread)
		{
			PlatformJoinThread(g_searchThread);
			g_hasSearchThread = false;
		}
		if (g_hasTimeLimitThread)
		{
			g_Stopped = true;
			PlatformJoinThread(g_timeLimitThread);
			g_hasTimeLimitThread = false;
		}
		if (!IsMoveValid(g_topSearchParams.BestMove))
			g_topSearchParams.BestMove = GetFirstValidMove(&g_mainGame, false);

		return CoordinatesFromMove(g_topSearchParams.BestMove);
	}

	// this will not be used.
	return CoordinatesFromMove(InvalidMove());
}
