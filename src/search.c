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

uchar lmr_matrix[42][100];

void InitLmr() {
	/*
	i > 3 & depth > 3 -- > 2
	i > 6 & depth > 6 -- > 3
	i > 9 & depth > 9 -- > 4
	*/
	for (size_t depth = 0; depth < 42; depth++)
	{
		for (size_t moveNo = 0; moveNo < 100; moveNo++)
		{
			if (depth > 12 && moveNo > 15)
				lmr_matrix[depth][moveNo] = 4;
			else if (depth > 7 && moveNo > 10)
				lmr_matrix[depth][moveNo] = 3;
			else
				lmr_matrix[depth][moveNo] = 2;
		}
	}
}
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

short QuiteSearch(short best_black, short best_white, Game* game, short moveScore, int deep_in) {

	g_SearchedNodes++;

	if (DrawByRepetition(game) || game->FiftyMoveRuleCount > 100)
		return 0;

	int score = GetEval(game, moveScore); // There seems to be a small advantage in taking time to fully evaluate even here.

	if (game->Side == BLACK) {
		if (score >= best_white)
			return best_white;
		if (score > best_black)
			best_black = score;
	}
	else {
		if (score <= best_black)
			return best_black;
		if (score < best_white)
			best_white = score;
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
			score = QuiteSearch(best_black, best_white, game, childMove.Score, deep_in + 1);
			UndoMove(game, childMove, undos);
			if (score > best_black) {
				if (score >= best_white) {
					free(localMoves);
					return best_white;
				}
				best_black = score;
			}
		}
		free(localMoves);
		return best_black;
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
			score = QuiteSearch(best_black, best_white, game, childMove.Score, deep_in + 1);
			UndoMove(game, childMove, undos);
			if (score < best_white) {
				if (score <= best_black) {
					free(localMoves);
					return best_black;
				}
				best_white = score;
			}
		}
		free(localMoves);
		return best_white;
	}
}


bool okToReduce(Move move, Undos undos) {
	return undos.CaptIndex == -1 && // no capture
		move.MoveInfo != PromotionQueen &&
		move.MoveInfo != SoonPromoting;
}

short RecursiveSearch(short best_black, short best_white, int depth, Game* game, bool doNull, short moveScore, int deep_in) {
	if (g_Stopped)
		return moveScore; // should not be used;

	g_SearchedNodes++;

	if (depth <= 0) {
		return QuiteSearch(best_black, best_white, game, moveScore, deep_in);
	}

	if (DrawByRepetition(game) || game->FiftyMoveRuleCount > 100)
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
	if (GetScoreFromHash(game->Hash, depth, &score, &pvMove, best_black, best_white)) {
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
				int nullScore = RecursiveSearch(best_black, best_black + 1, depth - r, game, false, moveScore, deep_in + 1);
				if (nullScore <= best_black && nullScore > -8000 && nullScore < 8000) { //todo, review if this is correct.
					UndoNullMove(prevState, game, prevHash);
					return best_black;
				}
			}
			else //(game->Side == WHITE)
			{
				int nullScore = RecursiveSearch(best_white - 1, best_white, depth - r, game, false, moveScore, deep_in + 1);
				if (nullScore >= best_white && nullScore > -8000 && nullScore < 8000) {
					UndoNullMove(prevState, game, prevHash);
					return best_white;
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

	//Not reducing for the first number of moves of each depth.
	const int fullDepthMoves = 3;
	//Not reducing when depth is or lower
	const int reductionLimit = 3;
	
	// alpha beta pruning
	short bestScore = 0;
	int legalCount = 0;
	short oldBestBlack = best_black;
	short oldBestWhite = best_white;
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
			
			int lmrRed = lmr_matrix[depth][i];
			// Late Move Reduction, full depth for the first moves, and interesting moves.
			if (i >= fullDepthMoves && depth >= reductionLimit && !incheck && okToReduce(childMove, undos))
				score = RecursiveSearch(best_black, best_black + 1, depth - lmrRed, game, true, childMove.Score, deep_in + 1);
			else
				score = best_black + 1;  // Hack to ensure that full-depth is done.

			if (score > best_black) { // surprisingly good, re-search att full depth.
				score = RecursiveSearch(best_black, best_white, depth - 1, game, true, childMove.Score, deep_in + 1);
			}
			
			UndoMove(game, childMove, undos);

			if (score > bestScore && !g_Stopped) {
				bestScore = score;
				bestMove = childMove;
				if (score > best_black) {
					if (score >= best_white) {
						AddHashScore(game->Hash, best_white, depth, BEST_WHITE, childMove.From, childMove.To);
						free(localMoves);

						/*if (undos.CaptIndex == -1) {
							game->KillerMoves[deep_in][1] = game->KillerMoves[deep_in][0];
							game->KillerMoves[deep_in][0] = childMove;
						}*/
						return best_white;
					}
					best_black = score;
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
			return best_black;

		if (best_black != oldBestBlack)
			AddHashScore(game->Hash, bestScore, depth, EXACT, bestMove.From, bestMove.To);
		else
			AddHashScore(game->Hash, best_black, depth, BEST_BLACK, bestMove.From, bestMove.To);

		return best_black;
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

			// late move reduction
			int lmrRed = lmr_matrix[depth][i];
			if (i >= fullDepthMoves && depth >= reductionLimit && !incheck && okToReduce(childMove, undos))
				score = RecursiveSearch(best_white - 1, best_white, depth - lmrRed, game, true, childMove.Score, deep_in + 1);
			else
				score = best_white - 1;  // Hack to ensure that full-depth  is done.

			if (score < best_white) {
				score = RecursiveSearch(best_black, best_white, depth - 1, game, true, childMove.Score, deep_in + 1);
			}			
			
			UndoMove(game, childMove, undos);

			if (score < bestScore && !g_Stopped) {
				bestScore = score;
				bestMove = childMove;
				if (score < best_white) {
					if (score <= best_black) {
						AddHashScore(game->Hash, best_black, depth, BEST_BLACK, bestMove.From, bestMove.To);
						/*if (undos.CaptIndex == -1) {
							game->KillerMoves[deep_in][1] = game->KillerMoves[deep_in][0];
							game->KillerMoves[deep_in][0] = childMove;
						}*/
						free(localMoves);
						return best_black;
					}
					best_white = score;
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
			return best_white;

		if (best_white != oldBestWhite)
			AddHashScore(game->Hash, bestScore, depth, EXACT, bestMove.From, bestMove.To);
		else
			AddHashScore(game->Hash, best_white, depth, BEST_WHITE, bestMove.From, bestMove.To);
		return best_white;
	}
}

void CopyMainGame(Game* copy) {

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

void PrintBestLine(Move move, int depth, float ellapsed) {
	char buffer[1800];
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
	PlayerMove moves[300];
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

		if ((now - start > (ms / (float)1000) * CLOCKS_PER_SEC))
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
		short score = RecursiveSearch(MIN_SCORE, MAX_SCORE, depth, pGame, true, 0, 0);
		if (g_Stopped)
			break;

		float ellapsed = (float)(clock() - start) / CLOCKS_PER_SEC;
		if (GetBestMoveFromHash(pGame->Hash, &bestMove))
		{
			bestMove.Score = score;
			if (depth > 3)
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
