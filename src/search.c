#include <stdlib.h>
#include <Windows.h>
#include "search.h"
#include "evaluation.h"
#include "moves.h"
#include "hashTable.h"
#include "timeControl.h"
#include <time.h>
#include <stdio.h>


void DefaultSearch() {
	g_topSearchParams.BlackIncrement = 0;
	g_topSearchParams.BlackTimeLeft = 0;
	g_topSearchParams.MaxDepth = 30;
	g_topSearchParams.MoveTime = 0;
	g_topSearchParams.TimeControl = false;
	g_topSearchParams.WhiteIncrement = 0;
	g_topSearchParams.BlackIncrement = 0;
	g_topSearchParams.MovesTogo = 0;
}

void MoveToTop(Move move, Move* list, int length) {
	for (size_t i = 0; i < length; i++)
	{
		if (move.From == list[i].From && move.To == list[i].To && i > 0) {
			Move temp = list[i];
			memmove(&list[1], list, i * sizeof(Move));
			list[0] = temp;
			return;
		}
	}
}

void MoveKillersToTop(Game* game, Move* moveList, int moveListLength, int deep_in) {
	Move secondKiller = game->KillerMoves[deep_in][1];
	for (size_t i = 0; i < moveListLength; i++)
	{
		if (moveList[i].From == secondKiller.From && moveList[i].To == secondKiller.To) {
			Move temp = moveList[i];
			memmove(&moveList[1], moveList, i * sizeof(Move));
			moveList[0] = temp;
			break;
		}
	}
	Move firstKiller = game->KillerMoves[deep_in][0];
	for (size_t i = 0; i < moveListLength; i++)
	{
		if (moveList[i].From == firstKiller.From && moveList[i].To == firstKiller.To) {
			Move temp = moveList[i];
			memmove(&moveList[1], moveList, i * sizeof(Move));
			moveList[0] = temp;
			break;
		}
	}
}

short AlphaBetaQuite(short alpha, short beta, Game* game, short moveScore, int deep_in) {

	g_SearchedNodes++;

	if (DrawByRepetition(game))
		return 0;

	int score = GetEval(game, moveScore); // There seems to be a small advantage in taking time to fully evaluate even here.

	if (game->Side == BLACK) {
		if (score >= beta)
			return beta;
		if (score > alpha)
			alpha = score;
	}
	else {
		if (score <= alpha)
			return alpha;
		if (score < beta)
			beta = score;
	}


	CreateCaptureMoves(game);
	int moveCount = game->MovesBufferLength;
	if (moveCount == 0)
		return score;

	Move* localMoves = malloc(moveCount * sizeof(Move));
	memcpy(localMoves, game->MovesBuffer, moveCount * sizeof(Move));
	MoveKillersToTop(game, localMoves, moveCount, deep_in);

	if (game->Side == BLACK) { //maximizing
		score = MIN_SCORE;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			GameState state = game->State;
			int prevPosScore = game->PositionScore;
			U64 prevHash = game->Hash;

			int captIndex = MakeMove(childMove, game);
			int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
			bool legal = !SquareAttacked(kingSquare, game->Side, game);
			if (!legal)
			{
				UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);
				continue;
			}
			score = AlphaBetaQuite(alpha, beta, game, childMove.Score, deep_in + 1);
			UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);
			if (score > alpha) {
				if (score >= beta) {
					free(localMoves);
					return beta;
				}
				alpha = score;
			}
		}
		free(localMoves);
		return alpha;
	}
	else { //minimizing
		score = MAX_SCORE;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			GameState state = game->State;
			int prevPosScore = game->PositionScore;
			U64 prevHash = game->Hash;

			int captIndex = MakeMove(childMove, game);
			int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
			bool legal = !SquareAttacked(kingSquare, game->Side, game);
			if (!legal)
			{
				UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);
				continue;
			}
			score = AlphaBetaQuite(alpha, beta, game, childMove.Score, deep_in + 1);
			UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);
			if (score < beta) {
				if (score <= alpha) {
					free(localMoves);
					return alpha;
				}
				beta = score;
			}
		}
		free(localMoves);
		return beta;
	}
}


int Reduction(int moveNo) {
	//removes one depth for moves n and later
	return min(moveNo / 15 + 1, 2);
}

short AlphaBeta(short alpha, short beta, int depth, int captIndex, Game* game, bool doNull, short moveScore, int deep_in) {
	if (g_Stopped)
		return moveScore; // should not be used;

	g_SearchedNodes++;

	if (depth <= 0) {
		return AlphaBetaQuite(alpha, beta, game, moveScore, deep_in);
	}

	if (DrawByRepetition(game))
		return 0;

	//In check extension
	int side01 = game->Side01;
	int otherSide = game->Side ^ 24;
	bool incheck = SquareAttacked(game->KingSquares[side01], otherSide, game);
	if (incheck)
		depth++;

	//Probe hash
	short score = 0; Move pvMove;
	pvMove.MoveInfo = NotAMove;
	if (getScoreFromHash(game->Hash, depth, &score, &pvMove, alpha, beta)) {
		return score;
	}

	//NULL move check
	int r = 3;
	if ((game->Side == WHITE && game->Material[side01] < -500) || // todo: check for pieces when piece list works
		(game->Side == BLACK && game->Material[side01] > 500))
	{
		if (doNull && !incheck && game->PositionHistoryLength && depth >= r) {
			GameState prevState = game->State;
			U64 prevHash = game->Hash;
			MakeNullMove(game);
			if (game->Side == BLACK) {
				int nullScore = AlphaBeta(alpha, alpha + 1, depth - r, captIndex, game, false, moveScore, deep_in + 1);
				if (nullScore <= alpha && nullScore > -8000 && nullScore < 8000) {
					UnMakeNullMove(prevState, game, prevHash);
					return alpha;
				}
			}
			else //(game->Side == WHITE)
			{
				int nullScore = AlphaBeta(beta - 1, beta, depth - r, captIndex, game, false, moveScore, deep_in + 1);
				if (nullScore >= beta && nullScore > -8000 && nullScore < 8000) {
					UnMakeNullMove(prevState, game, prevHash);
					return beta;
				}
			}
			UnMakeNullMove(prevState, game, prevHash);
		}
	}

	//Move generation
	CreateMoves(game, depth);
	int moveCount = game->MovesBufferLength;

	Move* localMoves = malloc(moveCount * sizeof(Move));
	memcpy(localMoves, game->MovesBuffer, moveCount * sizeof(Move));

	if (pvMove.MoveInfo != NotAMove) {
		MoveToTop(pvMove, localMoves, moveCount);
	}


	// alpha beta pruning
	short bestScore = 0;
	int legalCount = 0;
	short oldAlpha = alpha;
	short oldBeta = beta;
	Move bestMove;
	if (game->Side == BLACK) { //maximizing, black
		bestScore = MIN_SCORE;
		score = MIN_SCORE;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			GameState state = game->State;
			int prevPosScore = game->PositionScore;
			U64 prevHash = game->Hash;

			int captIndex = MakeMove(childMove, game);

			int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
			bool isLegal = !SquareAttacked(kingSquare, game->Side, game);
			if (!isLegal)
			{
				UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);
				continue;
			}
			legalCount++;
			//int red = Reduction(i); 
			score = AlphaBeta(alpha, beta, depth - 1, captIndex, game, true, childMove.Score, deep_in + 1);
			UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);

			if (score > bestScore && !g_Stopped) {
				bestScore = score;
				bestMove = childMove;
				if (score > alpha) {
					if (score >= beta) {
						addHashScore(game->Hash, beta, depth, BETA, childMove.From, childMove.To);
						free(localMoves);

						if (captIndex == -1) {
							game->KillerMoves[deep_in][1] = game->KillerMoves[deep_in][0];
							game->KillerMoves[deep_in][0] = childMove;
						}
						return beta;
					}
					alpha = score;
				}
			}
		}

		free(localMoves);
		if (legalCount == 0) {
			if (incheck)
				return -8000 + deep_in;
			else
				return 0;
		}

		if (g_Stopped)
			return alpha;

		if (alpha != oldAlpha)
			addHashScore(game->Hash, bestScore, depth, EXACT, bestMove.From, bestMove.To);
		else
			addHashScore(game->Hash, alpha, depth, ALPHA, bestMove.From, bestMove.To);

		return alpha;
	}
	else { //minimizing, white
		bestScore = MAX_SCORE;
		score = MAX_SCORE;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			GameState state = game->State;
			short prevPosScore = game->PositionScore;
			U64 prevHash = game->Hash;
			int captIndex = MakeMove(childMove, game);
			int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
			bool isLegal = !SquareAttacked(kingSquare, game->Side, game);
			if (!isLegal)
			{
				UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);
				continue;
			}
			legalCount++;
			//int red = Reduction(i);
			score = AlphaBeta(alpha, beta, depth - 1, captIndex, game, true, childMove.Score, deep_in + 1);
			UnMakeMove(childMove, captIndex, state, prevPosScore, game, prevHash);

			if (score < bestScore && !g_Stopped) {
				bestScore = score;
				bestMove = childMove;
				if (score < beta) {
					if (score <= alpha) {
						addHashScore(game->Hash, alpha, depth, ALPHA, bestMove.From, bestMove.To);
						if (captIndex == -1) {
							game->KillerMoves[deep_in][1] = game->KillerMoves[deep_in][0];
							game->KillerMoves[deep_in][0] = childMove;
						}
						free(localMoves);
						return alpha;
					}
					beta = score;
				}
			}
		}
		free(localMoves);
		if (legalCount == 0) {
			if (incheck)
				return 8000 - deep_in; //mate
			else
				return 0; //stale mate
		}

		if (g_Stopped)
			return beta;

		if (beta != oldBeta)
			addHashScore(game->Hash, bestScore, depth, EXACT, bestMove.From, bestMove.To);
		else
			addHashScore(game->Hash, beta, depth, BETA, bestMove.From, bestMove.To);
		return beta;
	}
}

Game* CopyMainGame(int threadNo) {
	g_threadGames[threadNo] = g_mainGame;
	g_threadGames[threadNo].KingSquares[0] = g_mainGame.KingSquares[0];
	g_threadGames[threadNo].KingSquares[1] = g_mainGame.KingSquares[1];
	g_threadGames[threadNo].Material[0] = g_mainGame.Material[0];
	g_threadGames[threadNo].Material[1] = g_mainGame.Material[1];
	g_threadGames[threadNo].ThreadIndex = threadNo;

	memcpy(g_mainGame.MovesBuffer, g_threadGames[threadNo].MovesBuffer, g_mainGame.MovesBufferLength * sizeof(Move));
	memcpy(g_mainGame.Squares, g_threadGames[threadNo].Squares, 64 * sizeof(PieceType));
	memcpy(g_mainGame.PositionHistory, g_threadGames[threadNo].PositionHistory, g_mainGame.PositionHistoryLength * sizeof(U64));
	memcpy(g_mainGame.Pieces, g_threadGames[threadNo].Pieces, 32 * sizeof(Piece));
	memset(g_threadGames[threadNo].KillerMoves, 0, 2 * 31 * sizeof(Move));
	return &g_threadGames[threadNo];
}

DWORD WINAPI DoNothingThread(int* prm) {
	Sleep(50);
	ExitThread(0);
}

int g_LastStartedMove = -1;
int GetNextFreeMove() {
	DWORD waitResult = WaitForSingleObject(g_MutexFreeMove, INFINITE);
	int ret;
	if (waitResult == WAIT_OBJECT_0) {
		ret = g_LastStartedMove + 1;
		g_LastStartedMove++;
		ReleaseMutex(g_MutexFreeMove);
	}
	else {
		printf("Unexpected waitResult (%d) in GetNextFreeMove.", waitResult);
		exit(999);
	}
	return ret;
}

// Entry point for a thread that starts the alphabeta tree search for a given depth and a given move.
// When finished takes next root move until they are no more.
// Sets the score on the root move. They are all common for all threads.
DWORD WINAPI SearchThread(ThreadParams* prm) {
	int moveIndex = GetNextFreeMove();
	do
	{
		Sleep(10);
		//if (prm->depth > 7)
			//printf("Start Thread %d on move %d\n", prm->threadID, moveIndex);

		Game* game = &(g_threadGames[prm->threadID]);
		Move move = g_rootMoves.moves[moveIndex];
		GameState gameState = game->State;
		int positionScore = game->PositionScore;
		U64 prevHash = game->Hash;

		int captIndex = MakeMove(move, game);
		short g_alpha = MIN_SCORE;
		short g_beta = MAX_SCORE;
		int score = AlphaBeta(g_alpha, g_beta, prm->depth, captIndex, game, true, move.Score, 0);

		if (!g_Stopped)
			g_rootMoves.moves[moveIndex].Score = score;
		UnMakeMove(move, captIndex, gameState, positionScore, game, prevHash);

		moveIndex = GetNextFreeMove();

	} while (moveIndex < prm->moveCount);
	ExitThread(0);
	return 0;
}

void SetMovesScoreAtDepth(int depth, int moveCount) {

	int moveIndex = 0;
	//Startar alla trådar för angivet djup.
	//En tråd per root move. när den är klar tar den nästa lediga root move.
	//När alla trådar är klara sorteras root moves oc det bästa skrivs ut.

	HANDLE threadHandles[SEARCH_THREADS];

	ThreadParams* tps = malloc(sizeof(ThreadParams) * SEARCH_THREADS);

	g_LastStartedMove = -1;

	for (int i = 0; i < SEARCH_THREADS; i++)
		CopyMainGame(i);

	for (int i = 0; i < SEARCH_THREADS; i++)
	{
		if (i > moveCount - 1) //in case more threads than moves
			threadHandles[i] = CreateThread(NULL, 0, DoNothingThread, NULL, 0, NULL);
		else {
			tps->threadID = i;
			tps->depth = depth;
			tps->moveCount = moveCount;
			threadHandles[i] = CreateThread(NULL, 0, SearchThread, tps, 0, NULL);
			tps++;

		}
	}
	WaitForMultipleObjects(SEARCH_THREADS, threadHandles, TRUE, INFINITE);
	SortMoves(g_rootMoves.moves, moveCount, g_mainGame.Side); //TODO: Why?
	//free(tps);
}

int PrintBestLine(Move move, int depth, float ellapsed) {
	char buffer[2000];
	char* pv = &buffer;
	char sMove[5];
	MoveToString(move, sMove);
	strcpy(pv, sMove);
	pv += 4;
	strcpy(pv, " ");
	pv++;
	Game* game = &g_mainGame;

	PlayerMove bestPlayerMove = MakePlayerMoveOnThread(game, sMove);
	int index = 0;
	PlayerMove moves[400];
	int movesCount = 0;
	while (movesCount < depth)
	{
		Move bestMove;
		if (!getBestMoveFromHash(game->Hash, &bestMove))
			break;

		char* sMove[5];
		MoveToString(bestMove, sMove);

		PlayerMove plMove = MakePlayerMoveOnThread(game, sMove);
		if (plMove.Invalid)
			break;
		moves[movesCount++] = plMove;
		strcpy(pv, sMove); pv += 4;
		strcpy(pv, " "); pv++;
	}
	strcpy(pv, "\0");

	for (int i = movesCount - 1; i >= 0; i--)
		UnMakePlayerMoveOnThread(game, moves[i]);
	UnMakePlayerMoveOnThread(game, bestPlayerMove);
	int nps = (float)g_SearchedNodes / ellapsed;
	int time = ellapsed * 1000;
	short score = move.Score;
	if (game->Side == WHITE)
		score = -score;
	printf("info score cp %d depth %d nodes %d time %d nps %d pv %s\n", score, depth, g_SearchedNodes, time, nps, buffer);
	fflush(stdout);
	return 0;
}

// Starting point of one thread that evaluates best score for every 7th root move. (If there are 7 threads)
// Increasing depth until given max depth.
DWORD WINAPI  BestMoveDeepening(void* v) {
	int maxDepth = g_topSearchParams.MaxDepth;
	clock_t start = clock();
	//ClearHashTable();
	CreateMoves(&g_mainGame, 0);
	RemoveInvalidMoves(&g_mainGame);
	int moveCount = g_mainGame.MovesBufferLength;
	g_rootMoves.Length = moveCount;
	memcpy(g_rootMoves.moves, g_mainGame.MovesBuffer, moveCount * sizeof(Move));

	int depth = 1;
	char bestMove[5];
	int bestScore;
	do
	{
		clock_t depStart = clock();
		SetMovesScoreAtDepth(depth, moveCount);
		if (!g_Stopped) { // avbrutna depths ger felaktigt resultat.
			g_topSearchParams.BestMove = g_rootMoves.moves[0];
			bestScore = g_rootMoves.moves[0].Score;
			MoveToString(g_rootMoves.moves[0], bestMove);
			depth++;
			float ellapsed = (float)(clock() - start) / CLOCKS_PER_SEC;
			PrintBestLine(g_rootMoves.moves[0], depth, ellapsed);
			if ((g_mainGame.Side == WHITE && bestScore < -7000) || (g_mainGame.Side == BLACK && bestScore > 7000))
			{
				g_Stopped = true;
				break; //A check mate is found, no need to search further.
			}

			float depthTime = (float)(clock() - depStart) / CLOCKS_PER_SEC;
			int moveNo = g_mainGame.PositionHistoryLength;

			RegisterDepthTime(moveNo, depth, depthTime * 1000);
			if (g_topSearchParams.TimeControl && !SearchDeeper(depth, moveNo, ellapsed * 1000, g_mainGame.Side)) {
				g_Stopped = true;
				break;
			}
		}

	} while (depth <= maxDepth && !g_Stopped);
	PrintHashStats();


	printf("bestmove %s\n", bestMove);
	fflush(stdout);

	ExitThread(0);
	return 0;
}
// Background thread that sets Stopped flag after specified time in ms.
DWORD WINAPI TimeLimitWatch(void* args) {
	int ms = g_topSearchParams.MoveTime;
	clock_t start = clock();
	clock_t now = clock();
	printf("TimeLimitWatch %d\n", ms);
	while (!g_Stopped)
	{
		Sleep(100);
		now = clock();

		if ((now - start > (ms / (float)1000)* CLOCKS_PER_SEC))
		{
			printf("Search stopped by timeout after %dms.\n", now - start); // This is not preferable since it is more economic to stop before going to next depth.
			fflush(stdout);
			break;
		}
	}

	g_Stopped = true;
	ExitThread(0);
	return 0;
}

// Starting point of a search for best move.
// Continues until time millis is reached or depth is reached.
// When async is set the result is printed to stdout. Not returned.
Move Search(bool async) {
	HANDLE timeLimitThread = 0;
	if (g_topSearchParams.MoveTime > 0) {
		timeLimitThread = CreateThread(NULL, 0, TimeLimitWatch, NULL, 0, NULL);
	}

	g_Stopped = false;
	g_SearchedNodes = 0;

	HANDLE handle = CreateThread(NULL, 0, BestMoveDeepening, NULL, 0, NULL);
	if (!async)
	{
		WaitForSingleObject(handle, INFINITE);
		if (timeLimitThread != 0)
			TerminateThread(timeLimitThread, 0);
		return g_topSearchParams.BestMove;
	}

	//this will not be used.
	Move nomove;
	nomove.MoveInfo = NotAMove;
	return nomove;
}