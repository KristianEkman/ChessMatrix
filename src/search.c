#include <stdlib.h>
#include <Windows.h>
#include "search.h"
#include "evaluation.h"
#include "moves.h"
#include "hashTable.h"
#include "timeControl.h"
#include "book.h"
#include <time.h>
#include <stdio.h>


void SetSearchDefaults() {
	g_topSearchParams.BlackIncrement = 0;
	g_topSearchParams.BlackTimeLeft = 0;
	g_topSearchParams.MaxDepth = 40;
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
	//MoveKillersToTop(game, localMoves, moveCount, deep_in);

	if (game->Side == BLACK) { //maximizing
		score = MIN_SCORE;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			Undos undos = DoMove(childMove, game);
			int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
			bool legal = !SquareAttacked(kingSquare, game->Side, game);
			if (!legal)
			{
				UndoMove(game, childMove, undos);
				continue;
			}
			score = AlphaBetaQuite(alpha, beta, game, childMove.Score, deep_in + 1);
			UndoMove(game, childMove, undos);
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
			Undos undos = DoMove(childMove, game);
			int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
			bool legal = !SquareAttacked(kingSquare, game->Side, game);
			if (!legal)
			{
				UndoMove(game, childMove, undos);
				continue;
			}
			score = AlphaBetaQuite(alpha, beta, game, childMove.Score, deep_in + 1);
			UndoMove(game, childMove, undos);
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

short AlphaBeta(short alpha, short beta, int depth, Game* game, bool doNull, short moveScore, int deep_in) {
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
	if (GetScoreFromHash(game->Hash, depth, &score, &pvMove, alpha, beta)) {
		return score;
	}

	//NULL move check
	int r = 3;
	if ((game->Side == WHITE && game->Material[side01] < -500) ||
		(game->Side == BLACK && game->Material[side01] > 500))
	{
		if (doNull && !incheck && game->PositionHistoryLength && depth >= r) {
			GameState prevState = game->State;
			U64 prevHash = game->Hash;
			DoNullMove(game);
			if (game->Side == BLACK) {
				int nullScore = AlphaBeta(alpha, alpha + 1, depth - r, game, false, moveScore, deep_in + 1);
				if (nullScore <= alpha && nullScore > -8000 && nullScore < 8000) {
					UndoNullMove(prevState, game, prevHash);
					return alpha;
				}
			}
			else //(game->Side == WHITE)
			{
				int nullScore = AlphaBeta(beta - 1, beta, depth - r, game, false, moveScore, deep_in + 1);
				if (nullScore >= beta && nullScore > -8000 && nullScore < 8000) {
					UndoNullMove(prevState, game, prevHash);
					return beta;
				}
			}
			UndoNullMove(prevState, game, prevHash);
		}
	}

	//Move generation
	CreateMoves(game);
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
	Move bestMove = localMoves[0];
	if (game->Side == BLACK) { //maximizing, black
		bestScore = MIN_SCORE;
		score = MIN_SCORE;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			Undos undos = DoMove(childMove, game);

			int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
			bool isLegal = !SquareAttacked(kingSquare, game->Side, game);
			if (!isLegal)
			{
				UndoMove(game, childMove, undos);
				continue;
			}
			legalCount++;
			//int red = Reduction(i); 
			score = AlphaBeta(alpha, beta, depth - 1, game, true, childMove.Score, deep_in + 1);
			UndoMove(game, childMove, undos);

			if (score > bestScore && !g_Stopped) {
				bestScore = score;
				bestMove = childMove;
				if (score > alpha) {
					if (score >= beta) {
						AddHashScore(game->Hash, beta, depth, BETA, childMove.From, childMove.To);
						free(localMoves);

						/*if (undos.CaptIndex == -1) {
							game->KillerMoves[deep_in][1] = game->KillerMoves[deep_in][0];
							game->KillerMoves[deep_in][0] = childMove;
						}*/
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
			AddHashScore(game->Hash, bestScore, depth, EXACT, bestMove.From, bestMove.To);
		else
			AddHashScore(game->Hash, alpha, depth, ALPHA, bestMove.From, bestMove.To);

		return alpha;
	}
	else { //minimizing, white
		bestScore = MAX_SCORE;
		score = MAX_SCORE;
		for (int i = 0; i < moveCount; i++)
		{
			Move childMove = localMoves[i];
			Undos undos = DoMove(childMove, game);
			int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
			bool isLegal = !SquareAttacked(kingSquare, game->Side, game);
			if (!isLegal)
			{
				UndoMove(game, childMove, undos);
				continue;
			}
			legalCount++;
			//int red = Reduction(i);
			score = AlphaBeta(alpha, beta, depth - 1, game, true, childMove.Score, deep_in + 1);
			UndoMove(game, childMove, undos);

			if (score < bestScore && !g_Stopped) {
				bestScore = score;
				bestMove = childMove;
				if (score < beta) {
					if (score <= alpha) {
						AddHashScore(game->Hash, alpha, depth, ALPHA, bestMove.From, bestMove.To);
						/*if (undos.CaptIndex == -1) {
							game->KillerMoves[deep_in][1] = game->KillerMoves[deep_in][0];
							game->KillerMoves[deep_in][0] = childMove;
						}*/
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
			AddHashScore(game->Hash, bestScore, depth, EXACT, bestMove.From, bestMove.To);
		else
			AddHashScore(game->Hash, beta, depth, BETA, bestMove.From, bestMove.To);
		return beta;
	}
}

void CopyMainGame(Game * copy) {

	copy->KingSquares[0] = g_mainGame.KingSquares[0];
	copy->KingSquares[1] = g_mainGame.KingSquares[1];
	copy->Material[0] = g_mainGame.Material[0];
	copy->Material[1] = g_mainGame.Material[1];

	memcpy(g_mainGame.MovesBuffer, copy->MovesBuffer, g_mainGame.MovesBufferLength * sizeof(Move));
	memcpy(g_mainGame.Squares, copy->Squares, 64 * sizeof(PieceType));
	memcpy(g_mainGame.PositionHistory, copy->PositionHistory, g_mainGame.PositionHistoryLength * sizeof(U64));
	memcpy(g_mainGame.Pieces, copy->Pieces, 32 * sizeof(Piece));
	memset(copy->KillerMoves, 0, 2 * 31 * sizeof(Move));
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
	PlayerMove moves[350];
	int movesCount = 0;
	while (movesCount < (depth + 10)) // hack: shows moves found when extending search (checks), but not going into cycles.
	{
		Move bestMove;
		if (!GetBestMoveFromHash(game->Hash, &bestMove))
			break;

		char sMove[5];
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
	
	printf("info score ");
	if (abs(score) > 7000) {
		bool meMated = false;
		if (score > 7000 && game->Side == WHITE || score < -7000 && game->Side == BLACK) {
			meMated = true;
		}
		int mateIn = (8000 - abs(score)) / 2 + 1;
		if (meMated)
			mateIn = -mateIn;
		printf("mate %d ", mateIn);
	}
	else {
		if (game->Side == WHITE)
			score = -score;
		printf("cp %d ", score);
	}

	printf("depth %d nodes %d time %d nps %d hashfull %d pv %s\n", depth, g_SearchedNodes, time, nps, HashFull(), buffer);
	fflush(stdout);
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


DWORD WINAPI IterativeSearch(void* v) {
	Game game = g_mainGame;
	Game* pGame = &game;
	CopyMainGame(pGame);
	clock_t start = clock();
	Move bestMove;
	bestMove.From = 0;
	bestMove.To = 0;
	bestMove.MoveInfo = NotAMove;

	for (int depth = 1; depth < g_topSearchParams.MaxDepth + 2; depth++)
	{
		clock_t depStart = clock();
		short score = AlphaBeta(MIN_SCORE, MAX_SCORE, depth, pGame, true, 0, 0);
		if (g_Stopped)
			break;

		float ellapsed = (float)(clock() - start) / CLOCKS_PER_SEC;
		if (GetBestMoveFromHash(pGame->Hash, &bestMove))
		{
			bestMove.Score = score;
			PrintBestLine(bestMove, depth, ellapsed);
		}
		g_topSearchParams.BestMove = bestMove;

		if ((pGame->Side == WHITE && score < -7000) || (pGame->Side == BLACK && score > 7000))
		{
			g_Stopped = true;
			break; //A check mate is found, no need to search further.
		}

		float depthTime = (float)(clock() - depStart) / CLOCKS_PER_SEC;
		int moveNo = pGame->PositionHistoryLength;
		RegisterDepthTime(moveNo, depth, depthTime * 1000);
		if (g_topSearchParams.TimeControl && !SearchDeeper(depth, moveNo, ellapsed * 1000, pGame->Side)) {
			g_Stopped = true;
			break;
		}
	}

	char sMove[5];
	MoveToString(bestMove, sMove);
	printf("bestmove %s\n", sMove);
	fflush(stdout);
	ExitThread(0);
	return 0;
}

// Starting point of a search for best move.
// Continues until time millis is reached or depth is reached.
// When async is set the result is printed to stdout. Not returned.
MoveCoordinates Search(bool async) {
	MoveCoordinates bookMove = BestBookMove(&g_mainGame);
	if (bookMove.From != -1) {
		char sMove[5];
		CoordinatesToString(bookMove, sMove);
		printf("bestmove %s\n", sMove);
		fflush(stdout);
		return bookMove;
	}

	HANDLE timeLimitThread = 0;
	if (g_topSearchParams.MoveTime > 0) {
		timeLimitThread = CreateThread(NULL, 0, TimeLimitWatch, NULL, 0, NULL);
	}

	g_Stopped = false;
	g_SearchedNodes = 0;

	HANDLE handle = CreateThread(NULL, 0, IterativeSearch, NULL, 0, NULL);
	if (!async)
	{
		WaitForSingleObject(handle, INFINITE);
		if (timeLimitThread != 0)
			TerminateThread(timeLimitThread, 0);
		MoveCoordinates coords;
		coords.From = g_topSearchParams.BestMove.From;
		coords.To = g_topSearchParams.BestMove.To;
		return coords;
	}

	//this will not be used.
	MoveCoordinates nomove;
	nomove.From = -1;
	nomove.To = -1;
	return nomove;
}
