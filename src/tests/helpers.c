#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "helpers.h"

#include "../main.h"
#include "../fen.h"
#include "../utils.h"
#include "../hashTable.h"
#include "../evaluation.h"
#include "../search.h"
#include "../moves.h"
#include "../platform.h"
#include "../bitboards.h"

#define MAX_REGISTERED_TESTS 256

static RegisteredTest g_registeredTests[MAX_REGISTERED_TESTS];
static int g_registeredTestCount = 0;

void RegisterTest(const char *name, RegisteredTestFunc func, TestSuite suite)
{
	if (g_registeredTestCount >= MAX_REGISTERED_TESTS)
	{
		fprintf(stderr, "Too many registered tests. Increase MAX_REGISTERED_TESTS.\n");
		exit(EXIT_FAILURE);
	}

	g_registeredTests[g_registeredTestCount].Name = name;
	g_registeredTests[g_registeredTestCount].Func = func;
	g_registeredTests[g_registeredTestCount].Suite = suite;
	g_registeredTestCount++;
}

void RunRegisteredTests(TestSuite suite)
{
	for (int i = 0; i < g_registeredTestCount; i++)
	{
		if (g_registeredTests[i].Suite == suite)
		{
			g_registeredTests[i].Func();
		}
	}
}

int _failedAsserts = 0;
PerftResult perftResult = {0};

void Assert(int goodResult, const char *msg)
{
	if (goodResult == 0)
	{
		printf("\n");
		PrintRed(msg);
		printf("\n");
		_failedAsserts++;
	}
}

void AssertNot(int result, const char *msg)
{
	if (result != 0)
	{
		printf("\n");
		PrintRed(msg);
		printf("\n");
		_failedAsserts++;
	}
}

void AssertAreEqual(const char *s1, const char *s2, const char *msg)
{
	if (strcmp(s1, s2))
	{
		PrintRed(msg);
		printf("\n");
		printf("Expected: %s\n", s1);
		printf("Actual:   %s\n", s2);
		_failedAsserts++;
	}
}

void AssertAreEqualInts(int expected, int actual, const char *msg)
{
	if (expected != actual)
	{
		printf("\n");
		PrintRed(msg);
		printf("\n");
		char str[24];
		snprintf(str, 24, "Expected %d", expected);
		PrintRed(str);
		printf("\n");
		snprintf(str, 24, "Actual   %d", actual);
		PrintRed(str);
		_failedAsserts++;
	}
}

void AssertAreEqualLongs(U64 expected, U64 actual, const char *msg)
{
	if (expected != actual)
	{
		printf("\n");
		PrintRed(msg);
		printf("\n");
		char str[24];
		snprintf(str, 24, "Expected %llu", expected);
		PrintRed(str);
		printf("\n");
		snprintf(str, 24, "Actual   %llu", actual);
		PrintRed(str);
		_failedAsserts++;
	}
}

void printPerftResults()
{
	printf("\nCaptures: %d\nCastles: %d\nChecks Mates: %d\nChecks: %d\nEn passants: %d\nPromotions %d\n",
		   perftResult.Captures, perftResult.Castles, perftResult.CheckMates, perftResult.Checks,
		   perftResult.Enpassants, perftResult.Promotions);
}

int Perft(int depth)
{
	if (depth == 0)
	{
		return 1;
	}
	int nodeCount = 0;
	CreateMoves(&g_mainGame);
	RemoveInvalidMoves(&g_mainGame);
	if (g_mainGame.MovesBufferLength == 0)
		return nodeCount;
	int count = g_mainGame.MovesBufferLength;
	Move *localMoves = malloc(count * sizeof(Move));
	memcpy(localMoves, g_mainGame.MovesBuffer, count * sizeof(Move));
	for (int i = 0; i < count; i++)
	{
		Move move = localMoves[i];
		PieceType capture = g_mainGame.Squares[move.To];
		if (depth == 1)
		{
			if (capture != NOPIECE)
				perftResult.Captures++;
			if (move.MoveInfo == EnPassantCapture)
				perftResult.Captures++;
			if (move.MoveInfo == CastleLong || move.MoveInfo == CastleShort)
				perftResult.Castles++;
			if (move.MoveInfo >= PromotionQueen && move.MoveInfo <= PromotionKnight)
				perftResult.Promotions++;
			if (move.MoveInfo == EnPassantCapture)
				perftResult.Enpassants++;
		}

		Undos undos = DoMove(move, &g_mainGame);
		nodeCount += Perft(depth - 1);
		UndoMove(&g_mainGame, move, undos);
	}
	free(localMoves);
	return nodeCount;
}

#ifdef _DEBUG

int perftSaveHashCount = 0;
int collisionCount = 0;
int hashSplits = 0;

typedef struct
{
	U64 Hash;
	char Fen[100];
} HashFen;

HashFen HashFenDb[3000000];
void PerftSaveHash(int depth)
{
	char hasHash = FALSE;
	char fen[100];
	WriteFen(fen);

	// checking all previous fens
	for (int i = 0; i < perftSaveHashCount; i++)
	{
		if (HashFenDb[i].Hash == g_mainGame.Hash)
		{
			hasHash = TRUE;

			if (strcmp(fen, HashFenDb[i].Fen))
			{ // same hash but different fen
				collisionCount++;
				printf("\ncollision %d:\n%s\n%s\nHash: %llu", collisionCount, fen, HashFenDb[i].Fen, g_mainGame.Hash);
			}
		}
		// also check that the fen does not exists with other hash
		if (strcmp(fen, HashFenDb[i].Fen) == 0)
		{
			if (HashFenDb[i].Hash != g_mainGame.Hash)
			{
				hashSplits++;
				printf("\nHash Splits: %d\nFen: %s\n", hashSplits, fen);
			}
		}
	}

	if (!hasHash)
	{
		// adding the hash and fen to last entry
		HashFenDb[perftSaveHashCount].Hash = g_mainGame.Hash;
		strcpy_s(HashFenDb[perftSaveHashCount].Fen, 100, fen);
		perftSaveHashCount++;
		if (perftSaveHashCount % 10000 == 0)
			printf("\n%d", perftSaveHashCount);
	}

	// below is regular Perft
	if (depth == 0)
		return;
	CreateMoves(&g_mainGame);
	RemoveInvalidMoves(&g_mainGame);
	if (g_mainGame.MovesBufferLength == 0)
		return;
	int count = g_mainGame.MovesBufferLength;
	Move *localMoves = malloc(count * sizeof(Move));
	memcpy(localMoves, g_mainGame.MovesBuffer, count * sizeof(Move));
	for (int i = 0; i < count; i++)
	{
		Move move = localMoves[i];
		GameState prevGameState = g_mainGame.State;
		U64 prevHash = g_mainGame.Hash;

		Undos undos = DoMove(move, &g_mainGame);
		PerftSaveHash(depth - 1);
		UndoMove(&g_mainGame, move, undos);
	}
	free(localMoves);
}

#endif

int PerftHashDb(int depth)
{
	if (depth == 0)
		return 1;
	int nodeCount = 0;
	CreateMoves(&g_mainGame);
	RemoveInvalidMoves(&g_mainGame);
	if (g_mainGame.MovesBufferLength == 0)
		return nodeCount;
	int count = g_mainGame.MovesBufferLength;
	Move *localMoves = malloc(count * sizeof(Move));
	memcpy(localMoves, g_mainGame.MovesBuffer, count * sizeof(Move));
	for (int i = 0; i < count; i++)
	{
		Move move = localMoves[i];
		Undos undos = DoMove(move, &g_mainGame);
		nodeCount += PerftHashDb(depth - 1);
		UndoMove(&g_mainGame, move, undos);
	}
	free(localMoves);
	return nodeCount;
}

int PerftTest(char *fen, int depth)
{
	ReadFen(fen);
	int perftCount = 0;
	for (int i = 0; i < 2; i++)
	{
		clock_t start = clock();
		perftResult.Captures = 0;
		perftResult.Castles = 0;
		perftResult.CheckMates = 0;
		perftResult.Checks = 0;
		perftResult.Enpassants = 0;
		perftResult.Promotions = 0;
		short startScore = TotalMaterial(&g_mainGame);
		perftCount = Perft(depth);
		AssertAreEqualInts(startScore, TotalMaterial(&g_mainGame), "Game material missmatch");
		clock_t stop = clock();
		float secs = (float)(stop - start) / CLOCKS_PER_SEC;
		printf("\n%.2fk moves/s\n", perftCount / (1000 * secs));
	}
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen, outFen, "Start and end FEN differ");
	return perftCount;
}

void TimedTest(int iterations, void (*func)(int))
{
	clock_t start = clock();
	(*func)(iterations);
	clock_t stop = clock();

	float secs = (float)(stop - start) / CLOCKS_PER_SEC;
	printf("\n%.2fk iterations/s\n", iterations / (1000 * secs));
}

bool MovesContains(Move *moves, int count, Move move)
{
	for (int i = 0; i < count; i++)
	{
		if (moves[i].From == move.From && moves[i].To == move.To && moves[i].MoveInfo == move.MoveInfo)
		{
			return true;
		}
	}
	return false;
}

void AssertBitboardsEqual(AllPieceBitboards expected, AllPieceBitboards actual, const char *msg)
{
	AssertAreEqualLongs(expected.WhitePieces, actual.WhitePieces, msg);
	AssertAreEqualLongs(expected.BlackPieces, actual.BlackPieces, msg);
	AssertAreEqualLongs(expected.Occupied, actual.Occupied, msg);
	AssertAreEqualLongs(expected.Pawns.WhitePawns, actual.Pawns.WhitePawns, msg);
	AssertAreEqualLongs(expected.Pawns.BlackPawns, actual.Pawns.BlackPawns, msg);
	AssertAreEqualLongs(expected.Pawns.AllPawns, actual.Pawns.AllPawns, msg);
	AssertAreEqualLongs(expected.Knights.WhiteKnights, actual.Knights.WhiteKnights, msg);
	AssertAreEqualLongs(expected.Knights.BlackKnights, actual.Knights.BlackKnights, msg);
	AssertAreEqualLongs(expected.Knights.AllKnights, actual.Knights.AllKnights, msg);
	AssertAreEqualLongs(expected.Bishops.WhiteBishops, actual.Bishops.WhiteBishops, msg);
	AssertAreEqualLongs(expected.Bishops.BlackBishops, actual.Bishops.BlackBishops, msg);
	AssertAreEqualLongs(expected.Bishops.AllBishops, actual.Bishops.AllBishops, msg);
	AssertAreEqualLongs(expected.Rooks.WhiteRooks, actual.Rooks.WhiteRooks, msg);
	AssertAreEqualLongs(expected.Rooks.BlackRooks, actual.Rooks.BlackRooks, msg);
	AssertAreEqualLongs(expected.Rooks.AllRooks, actual.Rooks.AllRooks, msg);
	AssertAreEqualLongs(expected.Queens.WhiteQueens, actual.Queens.WhiteQueens, msg);
	AssertAreEqualLongs(expected.Queens.BlackQueens, actual.Queens.BlackQueens, msg);
	AssertAreEqualLongs(expected.Queens.AllQueens, actual.Queens.AllQueens, msg);
	AssertAreEqualLongs(expected.Kings.WhiteKing, actual.Kings.WhiteKing, msg);
	AssertAreEqualLongs(expected.Kings.BlackKing, actual.Kings.BlackKing, msg);
	AssertAreEqualLongs(expected.Kings.AllKings, actual.Kings.AllKings, msg);

	for (int pieceType = PAWN; pieceType <= KING; pieceType++)
	{
		for (int side = 0; side < 2; side++)
		{
			AssertAreEqualLongs(expected.Matrix[pieceType][side], actual.Matrix[pieceType][side], msg);
		}
	}
}

void AssertCachedBitboardsMatchGame(const char *msg)
{
	AssertBitboardsEqual(GetAllPieceBitboards(&g_mainGame), g_mainGame.Bitboards, msg);
}

void AssertBestMove(int depth, const char *testName, const char *fen, const char *expected)
{
	printf("\n\n****   %s   ****\n", testName);
	ReadFen(fen);
	ClearHashTable();
	g_SearchedNodes = 0;
	clock_t start = clock();
	SetSearchDefaults();
	g_topSearchParams.MaxDepth = depth;
	MoveCoordinates bestMove = Search(false);
	clock_t stop = clock();
	float secs = (float)(stop - start) / CLOCKS_PER_SEC;
	if (g_SearchedNodes == 0)
		printf("(forced move, no search needed)\n");
	else
	{
		printf("%.2fk leafs in %.2fs\n", (float)g_SearchedNodes / 1000, secs);
		printf("%.2fk leafs/s\n", g_SearchedNodes / (1000 * secs));
	}
	char sMove[6];
	CoordinatesToString(bestMove, sMove);
	AssertAreEqual(expected, sMove, "Not the expected move");
}

void AssertBestMoveTimed(int ms, const char *testName, const char *fen, const char *expected)
{
	printf("\n\n****   %s  (timed) ****\n", testName);
	ReadFen(fen);
	ClearHashTable();
	g_SearchedNodes = 0;
	SetSearchDefaults();
	g_topSearchParams.MoveTime = ms;
	MoveCoordinates bestMove = Search(false);
	char sMove[6];
	CoordinatesToString(bestMove, sMove);
	AssertAreEqual(expected, sMove, "Not the expected move");
}