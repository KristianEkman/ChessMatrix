#include <stdio.h>
#include <time.h>
#include <string.h>
#include <Windows.h>
#include <conio.h>

#include "main.h"
#include "commons.h"
#include "utils.h"
#include "hashTable.h"
#include "evaluation.h"
#include "search.h"
#include "moves.h"

#pragma region TestsHelpers

int _failedAsserts = 0;
PerftResult perftResult;

void Assert(int goodResult, char * msg) {
	if (goodResult == 0)
	{
		printf("\n");
		PrintRed(msg);
		printf("\n");
		_failedAsserts++;
	}
}

void AssertNot(int result, char * msg) {
	if (result != 0)
	{
		printf("\n");
		PrintRed(msg);
		printf("\n");
		_failedAsserts++;
	}
}

void AssertAreEqual(char * s1, char * s2, char * msg) {
	if (strcmp(s1, s2))
	{
		PrintRed(msg);
		printf("\n");
		printf("Expected: %s\n", s1);
		printf("Actual:   %s\n", s2);
		_failedAsserts++;
	}
}

void AssertAreEqualInts(int expected, int actual, char * msg) {
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

void AssertAreEqualLongs(U64 expected, U64 actual, char* msg) {
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

void printPerftResults() {
	printf("\nCaptures: %d\nCastles: %d\nChecks Mates: %d\nChecks: %d\nEn passants: %d\nPromotions %d\n",
		perftResult.Captures, perftResult.Castles, perftResult.CheckMates, perftResult.Checks,
		perftResult.Enpassants, perftResult.Promotions);
}

#pragma endregion

void HashKeyTest() {
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
	U64 hash1 = g_mainGame.Hash;
 	PlayerMove pl1 = MakePlayerMove("g1f3");
	PlayerMove pl2 = MakePlayerMove("g8f6");
	PlayerMove pl3 = MakePlayerMove("f3g1");
	PlayerMove pl4 = MakePlayerMove("f6g8");
	U64 hash2 = g_mainGame.Hash;

	AssertAreEqualLongs(hash1, hash2, "Hash keys should be equal");

}

void HashTableRoundTrip() {
	printf("%s\n", __func__);
	ClearHashTable();
	U64 hash = 0x1234567890ABCDEF;
	short expected = 3000;
	AddHashScore(hash, expected, 1, EXACT, 1, 2);
	int depth = 0;
	short score = 0;
	Move move;
	GetScoreFromHash(hash, 1, &score, &move, 3000 ,0);
	AssertAreEqualInts(expected, score, "hash table score missmatch");

	U64 hash2 = hash + 1;
	short expected2 = -4000;
	AddHashScore(hash2, expected2, 1, EXACT, 10, 12);

	short score2;
	GetScoreFromHash(hash2, 1, &score2, &move, 0 ,0);
	AssertAreEqualInts(expected2, score2, "hash table score missmatch");
	
	GetScoreFromHash(hash, 1, &score, &move, 0, 0);
	AssertAreEqualInts(expected, score, "hash table score missmatch");
}

void HashTableDepthTest() {
	printf("%s\n", __func__);
	ClearHashTable();
	U64 hash = 0x1234567890ABCDEF;

	AddHashScore(hash, 3000, 2, EXACT, 40, 50 );
	int depth = 2;
	short score;
	Move move;
	GetScoreFromHash(hash, depth, &score, &move, 100, 200);
	AssertAreEqualInts(3000, score, "hash table score missmatch");

	AddHashScore(hash, 4000, 1, EXACT, 10, 10); //smaller depth
	short score2 = 0;
	GetScoreFromHash(hash, 2,  &score2, &move, 30, 60);
	AssertAreEqualInts(3000, score2, "smaller depth should not replace score");

	AddHashScore(hash, 5000, 3, EXACT, 20, 30); //smaller depth
	GetScoreFromHash(hash, 3, &score, &move, 30, 31);
	AssertAreEqualInts(5000, score, "larger depth should replace value");
}

void HashTablePerformance(int iterations) {
	printf("%s\n", __func__);
	ClearHashTable();
	U64 hash = Llrand();
	short expected = MIN_SCORE;
	int depth = 1;
	short score = 0;
	Move move;

	for (int i = 0; i < iterations; i++)
	{
		expected++;
		if (expected > MAX_SCORE)
			expected = MIN_SCORE;
		hash++;
		AddHashScore(hash, expected, 1, EXACT, 1, 1);		
		Assert(GetScoreFromHash(hash, depth, &score, &move, 100, 200), "No score returned from hash");
		AssertAreEqualInts(expected, score, "hash table score missmatch");
	}
}
int Perft(depth) {
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
	Move * localMoves = malloc(count * sizeof(Move));
	memcpy(localMoves, g_mainGame.MovesBuffer, count * sizeof(Move));
	for (int i = 0; i < count; i++)
	{
		Move move = localMoves[i];
		PieceType capture = g_mainGame.Squares[move.To];
		if (depth == 1) {
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

typedef struct {
	U64 Hash;
	char Fen[100];
} HashFen;

HashFen HashFenDb[3000000];
void PerftSaveHash(depth) {
	char hasHash = FALSE;
	char fen[100];
	WriteFen(fen);

	// checking all previous fens
	for (size_t i = 0; i < perftSaveHashCount; i++)
	{
		if (HashFenDb[i].Hash == g_mainGame.Hash) {
			hasHash = TRUE;
			
			if (strcmp(fen, HashFenDb[i].Fen)) { // same hash but different fen
				collisionCount++;
				printf("\ncollision %d:\n%s\n%s\nHash: %llu", collisionCount ,fen, HashFenDb[i].Fen, g_mainGame.Hash);
			}
		}
		// also check that the fen does not exists with other hash
		if (strcmp(fen, HashFenDb[i].Fen) == 0 ) {
			if (HashFenDb[i].Hash != g_mainGame.Hash) {
				hashSplits++;
				printf("\nHash Splits: %d\nFen: %s\n", hashSplits, fen);
			}
		}
	}

	

	if (!hasHash) {
		// adding the hash and fen to last entry
		HashFenDb[perftSaveHashCount].Hash = g_mainGame.Hash;
		strcpy_s(HashFenDb[perftSaveHashCount].Fen, 100, fen);
		perftSaveHashCount++;
		if (perftSaveHashCount % 10000 == 0)
			printf("\n%d", perftSaveHashCount);
	}

	//below is regular Perft
	if (depth == 0)
		return;
	CreateMoves(&g_mainGame);
	RemoveInvalidMoves(&g_mainGame);
	if (g_mainGame.MovesBufferLength == 0)
		return;
	int count = g_mainGame.MovesBufferLength;
	Move * localMoves = malloc(count * sizeof(Move));
	memcpy(localMoves, g_mainGame.MovesBuffer, count * sizeof(Move));
	for (int i = 0; i < count; i++)
	{
		Move move = localMoves[i];
		GameState prevGameState = g_mainGame.State;
		int prevPositionScore = g_mainGame.PositionScore;
		U64 prevHash = g_mainGame.Hash;

		Undos undos  = DoMove(move, &g_mainGame);
		PerftSaveHash(depth - 1);
		UndoMove(&g_mainGame, move, undos);
	}
	free(localMoves);
}


void PerftSaveHashTest() {
	printf("%s\n", __func__);
	perftSaveHashCount = 0;
	collisionCount = 0;
	ReadFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"); //quite complicated position
	PerftSaveHash(3);
	printf("\nHash entries: %d\n", perftSaveHashCount);
	printf("\nCollisions: %d\n", collisionCount);
}

void EnpassantCollisionsTest() {
	printf("%s\n", __func__);
	perftSaveHashCount = 0;
	collisionCount = 0;
	ReadFen("rnbqkbnr/ppp2pp1/3p4/1P2p3/7p/3PP3/P1P2PPP/RNBQKBNR w KQkq - 0 1"); //quite complicated position
	PerftSaveHash(3);
	printf("\nHash entries: %d\n", perftSaveHashCount);
	printf("\nCollisions: %d\n", collisionCount);
}

//

#endif // _DEBUG

int PerftHashDb(int depth) {
	if (depth == 0)
		return 1;
	int nodeCount = 0;
	CreateMoves(&g_mainGame);
	RemoveInvalidMoves(&g_mainGame);
	if (g_mainGame.MovesBufferLength == 0)
		return nodeCount;
	int count = g_mainGame.MovesBufferLength;
	Move * localMoves = malloc(count * sizeof(Move));
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

int PerftTest(char * fen, int depth) {

	ReadFen(fen);
	//PrintGame(&mainGame);
	int perftCount = 0;
	for (size_t i = 0; i < 2; i++)
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
		//printPerftResults(perftResult);
		clock_t stop = clock();
		float secs = (float)(stop - start) / CLOCKS_PER_SEC;
		//printf("%.2fs\n", secs);
		//printf("%d moves\n", perftCount);
		printf("\n%.2fk moves/s\n", perftCount / (1000 * secs));

		//PrintGame(&mainGame);
	}
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen, outFen, "Start and end FEN differ");
	return perftCount;
}


void PerftHashDbTest() {
	printf("%s\n", __func__);
	ClearHashTable();
	ReadFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
	PerftHashDb(4);
	PrintHashStats();
}
void FenTest() {
	char fen1[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 1";
	ReadFen(fen1);
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen1, outFen, "Start and end fen differ");
}

void FenTestHalfMoves() {
	char fen1[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 23";
	ReadFen(fen1);
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen1, outFen, "Start and end fen differ");
}

void FenEnppasantTest() {
	printf("%s\n", __func__);
	ReadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	MakePlayerMove("d2d4");
	char fen[100];
	WriteFen(fen);
	AssertAreEqual(fen, "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0", "Fen missmatch");

	PlayerMove pm2 = MakePlayerMove("d7d6");
	UnMakePlayerMove(pm2);
	WriteFen(fen);
	AssertAreEqual(fen, "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0", "Fen missmatch");
}

void TimedTest(int iterations, void(*func)(int)) {
	clock_t start = clock();
	(*func)(iterations);
	clock_t stop = clock();

	float secs = (float)(stop - start) / CLOCKS_PER_SEC;	
	printf("\n%.2fk iterations/s\n", iterations / (1000 * secs));
}

void PerfTestPosition2() {
	printf("%s\n", __func__);
	char fen[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0";
	PerftTest(fen, 4);
	AssertAreEqualInts(757163, perftResult.Captures, "Captures missmatch");
	AssertAreEqualInts(128013, perftResult.Castles, "Castles missmatch");
	AssertAreEqualInts(1929, perftResult.Enpassants, "En passants missmatch");
	AssertAreEqualInts(15172, perftResult.Promotions, "Promotion missmatch");
}

void PerftTestStart() {
	printf("%s\n", __func__);
	char startFen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0";
	int count = PerftTest(startFen, 5);
	AssertAreEqualInts(4865609, count, "Perft Count missmatch");
}

bool MovesContains(Move * moves, int count, Move move) {
	for (int i = 0; i < count; i++)
	{
		if (moves[i].From == move.From && moves[i].To == move.To && moves[i].MoveInfo == move.MoveInfo) {
			return true;
		}
	}
	return false;
}

void ValidMovesPromotionCaptureAndCastling() {
	printf("%s\n", __func__);
	char * fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
	Move moves[100];
	ReadFen(fen);
	int count = ValidMoves(moves);
	AssertAreEqualInts(44, count, "Moves count missmatch");
	Move expectedMove;
	expectedMove.From = 4;
	expectedMove.To = 6;
	expectedMove.MoveInfo = CastleShort;
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
}

void LongCastling() {
	printf("%s\n", __func__);
	char * fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
	Move moves[100];
	ReadFen(fen);
	int count = ValidMoves(moves);
	AssertAreEqualInts(48, count, "Moves count missmatch");
	Move expectedMove;
	expectedMove.From = 4;
	expectedMove.To = 2;
	expectedMove.MoveInfo = CastleLong;
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
}

void EnPassantFromFenTest() {
	printf("%s\n", __func__);
	char * fen = "8/5k2/8/3Pp3/8/8/8/4K3 w - e6 0 3";
	ReadFen(fen);
	Move moves[100];
	int count = ValidMoves(moves);
	Move expectedMove = ParseMove("d5e6", EnPassantCapture);
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
	int startGameScore = TotalMaterial(&g_mainGame);
	AssertNot(MakePlayerMove("d5e6").Invalid, "Invalid move");
	AssertAreEqualInts(startGameScore - 100, TotalMaterial(&g_mainGame), "Material should decrease by 100");
}

void EnPassantAfterMove() {
	printf("%s\n", __func__);
	char * fen = "4k3/4p3/8/3P4/8/8/8/4K3 b - e3 0 1";
	ReadFen(fen);
	AssertNot(MakePlayerMove("e7e5").Invalid, "Move was not valid");

	Move moves[100];
	int count = ValidMoves(moves);
	Move expectedMove = ParseMove("d5e6", EnPassantCapture);
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
}


void BlackCastlingRightsAfterKingMove() {
	printf("%s\n", __func__);
	char* fen = "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/P4N2/1PPP1PPP/RNBQK2R w KQkq - 1 5";
	ReadFen(fen);
	AssertNot(MakePlayerMove("e1f1").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("e8f8").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("f1e1").Invalid, "Move was not valid");

	Move moves[100];
	int count = ValidMoves(moves);
	Move expectedMove = ParseMove("e8g8", CastleShort);
	AssertNot(MovesContains(moves, count, expectedMove), "Invalid move was found");
}

void WhiteCastlingRightsAfterKingMove() {
	printf("%s\n", __func__);
	char* fen = "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/P4N2/1PPP1PPP/RNBQK2R w KQkq - 1 5";
	ReadFen(fen);
	AssertNot(MakePlayerMove("e1f1").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("e8f8").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("f1e1").Invalid, "Move was not valid");
	AssertNot(MakePlayerMove("f8e8").Invalid, "Move was not valid");

	Move moves[100];
	int count = ValidMoves(moves);
	Move expectedMove = ParseMove("e1g1", CastleShort);
	AssertNot(MovesContains(moves, count, expectedMove), "Invalid move was found");
}

void MaterialBlackPawnCapture() {
	printf("%s\n", __func__);
	ReadFen("2r1k3/8/8/4p3/3P4/8/8/2Q1K3 w - - 0 1");
	AssertAreEqualInts(-510, TotalMaterial(&g_mainGame), "Start Material missmatch");
	AssertNot(MakePlayerMove("d4e5").Invalid, "Move was not valid");
	AssertAreEqualInts(-610, TotalMaterial(&g_mainGame), "Game Material missmatch");
}

void MaterialWhiteQueenCapture() {
	printf("%s\n", __func__);
	ReadFen("rnbqkbnr/ppp1pppp/8/3p4/4Q3/4P3/PPPP1PPP/RNB1KBNR b KQkq - 0 1");
	AssertAreEqualInts(0, TotalMaterial(&g_mainGame), "Start Material missmatch");
	AssertNot(MakePlayerMove("d5e4").Invalid, "Move was not valid");
	AssertAreEqualInts(1010, TotalMaterial(&g_mainGame), "Game Material missmatch");
}

void MaterialCaptureAndPromotion() {
	printf("%s\n", __func__);
	ReadFen("2r1k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
	AssertAreEqualInts(400, TotalMaterial(&g_mainGame), "Start Material missmatch");
	PlayerMove pm = MakePlayerMove("b7c8");
	AssertNot(pm.Invalid, "Move was not valid");
	AssertAreEqualInts(-1010, TotalMaterial(&g_mainGame), "Game Material missmatch");
	UnMakePlayerMove(pm);
	AssertAreEqualInts(400, TotalMaterial(&g_mainGame), "Start Material missmatch");
}

void MaterialPromotion() {
	printf("%s\n", __func__);
	ReadFen("2r1k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
	AssertAreEqualInts(400, TotalMaterial(&g_mainGame), "Start Material missmatch");
	AssertNot(MakePlayerMove("b7b8").Invalid, "Move was not valid");
	AssertAreEqualInts(-510, TotalMaterial(&g_mainGame), "Game Material missmatch");
}

void EnPassantMaterial() {
	printf("%s\n", __func__);
	ReadFen("r3k3/3p4/8/4P3/8/8/8/4K2R b - - 0 1");
	AssertAreEqualInts(0, TotalMaterial(&g_mainGame), "Start Material missmatch");
	AssertNot(MakePlayerMove("d7d5").Invalid, "Move was not valid");
	PlayerMove nextMove = MakePlayerMove("e5d6");
	AssertNot(nextMove.Invalid, "Move was not valid");
	AssertAreEqualInts(-100, TotalMaterial(&g_mainGame), "Game Material missmatch");
	UnMakePlayerMove(nextMove);
	AssertAreEqualInts(0, TotalMaterial(&g_mainGame), "Game Material missmatch");
}

void AssertBestMove(int depth, char * testName, char * fen, char * expected) {
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
	printf("%.2fk leafs in %.2fs\n", (float)g_SearchedNodes / 1000, secs);
	printf("%.2fk leafs/s\n", g_SearchedNodes / (1000 * secs));
	char sMove[6];
	CoordinatesToString(bestMove, sMove);
	AssertAreEqual(expected, sMove, "Not the expected move");
	/*printf("\nEntries:    %d", HashTableEntries);
	printf("\nMatches:    %d", HashTableMatches);
	printf("\nFull count: %d", HashTableFullCount);*/
}

void AssertBestMoveTimed(int ms, char* testName, char* fen, char* expected) {
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
	/*printf("\nEntries:    %d", HashTableEntries);
	printf("\nMatches:    %d", HashTableMatches);
	printf("\nFull count: %d", HashTableFullCount);*/
}

void BestMoveTestBlackCaptureBishop() {
	AssertBestMove(4, __func__, "r1bqk2r/ppp1bppp/2n1pn2/3p4/2BP1B2/2N1PN2/PPP2PPP/R2QK2R b KQkq - 2 6", "d5c4");
	//AssertBestMoveTimed(10, __func__, "r1bqk2r/ppp1bppp/2n1pn2/3p4/2BP1B2/2N1PN2/PPP2PPP/R2QK2R b KQkq - 2 6", "d5c4");

}

void TestWhiteMateIn2() {
	char * fen = "5k2/8/2Q5/3R4/8/8/8/4K3 w - - 2 1";
	AssertBestMove(5, __func__, fen, "d5d7");
	AssertBestMoveTimed(1000, __func__, fen, "d5d7");
}

void BlackMatesIn5Deeping() {
	char * fen = "1k2r3/pP3pp1/8/3P1B1p/5q2/N1P2b2/PP3Pp1/R5K1 b - - 0 1";
	AssertBestMoveTimed(1000, __func__, fen, "f4h4");

}

void BestMoveByWhite1() {
	char * fen = "r1bqkb1r/ppp1pppp/2npn3/4P3/2P5/2N2NP1/PP1P1P1P/R1BQKB1R w KQkq - 1 1";
	AssertBestMove(7, __func__, fen, "d2d4"); //to realy find the advantage of this move it takes about 10 - 15 moves deep.

	//AssertBestMoveTimed(10, __func__, fen, "d2d4");
}

void BestMoveByBlack2() {
	char * fen = "r1r5/1p6/2kpQ3/3p4/n2P4/4P3/3q1PPP/R4RK1 b - - 0 21";
	AssertBestMove(7, __func__, fen, "a4c3");
}

void BestMoveByBlack3() {
	char * fen = "8/kp6/8/3p4/3PnQ2/4P1P1/r2q1P1P/5RK1 b - - 2 27";
	AssertBestMove(7, __func__, fen, "d2e2");
}

//void BestMoveByWhite2() {
//	char * fen = "rn1r2k1/pp3ppp/8/3q4/3N4/P3P3/4QPPP/3R1RK1 w - - 1 19";
//	//requires atlest depth 7 to be found
//	AssertBestMove(7, __func__, fen, "d4f5");
//}

void BestMoveByBlack1() {
	char * fen = "r1bq2k1/p1p2pp1/2p2n1p/3pr3/7B/P1PBPQ2/5PPP/R4RK1 b - - 0 1";
	AssertBestMove(6, __func__, fen, "g7g5");
}

void BestMoveByBlack4() {
	char * fen = "r1b2r2/2q2pk1/2pb3p/pp2pNpn/4Pn2/P1NB2BP/1PP1QPP1/R4RK1 b - - 0 1";
	AssertBestMove(5, __func__, fen, "c8f5");
}

void BestMoveByBlack5() {
	char * fen = "r2qk2r/1b3pp1/pb2p2p/Rp2P3/2pPB3/2P2N2/2Q2PPP/2B2RK1 b - - 0 1";
	AssertBestMove(6, __func__, fen, "b7e4");
}

void BestMoveByWhite3() {
	char * fen = "r4rk1/p7/1p1N3p/3nPppb/3n4/3B3P/PP1B2K1/R4R2 w - - 0 1";
	AssertBestMove(5, __func__, fen, "d3c4");
}

void RookSacrificeByWhite() {
	char* fen = "r2q2k1/p4p1p/1rp3bB/3p4/3P1Q2/RP3P2/1KP5/4R3 w - - 3 47";
	AssertBestMoveTimed(2000, __func__, fen, "e1e8");
}

void BlackMatesIn5a() {
	char* fen = "6k1/3b3r/1p1p4/p1n2p2/1PPNpP1q/P3Q1p1/1R1RB1P1/5K2 b - -";
	AssertBestMoveTimed(500, __func__, fen, "h4f4");
}

void WhiteMatesIn5b() {
	char* fen = "2q1nk1r/4Rp2/1ppp1P2/6Pp/3p1B2/3P3P/PPP1Q3/6K1 w";
	AssertBestMove(11, __func__, fen, "e7e8");
}

void WhiteMatesIn7() {
	char* fen = "rn3rk1/pbppq1pp/1p2pb2/4N2Q/3PN3/3B4/PPP2PPP/R3K2R w KQ - 7 11";
	AssertBestMove(15, __func__, fen, "h5h7");
}

void EngineMated() {
	char* fen = "rn3rk1/pbppq1pQ/1p2pb2/4N3/3PN3/3B4/PPP2PPP/R3K2R b KQ - 0 11";
	AssertBestMove(8, __func__, fen, "g8h7");
}

void DeepTest() {
	printf("%s\n", __func__);
	char* fen = "r1b1k2r/ppppnppp/2n2q2/2b5/3NP3/2P1B3/PP3PPP/RN1QKB1R w KQkq - 0 1";
	AssertBestMove(7, __func__, fen, "b1d2");
}

void MobilityRookTest() {
	printf("%s\n", __func__);
	char * fen = "r3kbnr/ppp1pppp/2nb4/8/2P5/2N5/PP2PPPP/R1BRKB2 w Qkq - 0 1";
	ReadFen(fen);
	g_topSearchParams.MaxDepth = 1;
	Search(true); // mobility is calculated in movegenearion and alphabeta ... incheck
	short score = GetEval(&g_mainGame);
	// no asserts, just entrypoint for debugging.
}


void DoublePawnsTest()
{
	printf("%s\n", __func__);
	char* fen = "r3kbnr/pp2pppp/2np4/8/2P5/2N1P3/PP2P1PP/R1BRKB2 w Qkq - 0 1";
	ReadFen(fen);
	short score = DoublePawns(20, &g_mainGame, PAWN | WHITE);
	AssertAreEqualInts(9, score, "Double pawns score missmatch");
	AssertAreEqualInts(0, DoublePawns(8, &g_mainGame, PAWN | WHITE), "Double pawns score missmatch");
}

void KingExposureTest() {
	printf("%s\n", __func__);
	char * protected = "rnbq1rk1/pppppppp/4bn2/8/8/3B1N2/PPPPPPPP/RNBQ1RK1 w - - 0 1";
	ReadFen(protected);
	AssertAreEqualInts(6, g_mainGame.KingSquares[0], "White king is not on square 1");
	AssertAreEqualInts(62, g_mainGame.KingSquares[1], "Black king is not on square 62");

	short whiteScore = KingExposed(1, &g_mainGame);
	short blackScore = KingExposed(62, &g_mainGame);
	AssertAreEqualInts(0, whiteScore, "Not the expected exposure score for white");
	AssertAreEqualInts(0, blackScore, "Not the expected exposure score for black");

	char * unprotected = "rnbq1rk1/ppppp3/4b3/8/8/3B4/1PPPP3/RNBQ1RK1 w - - 0 1";
	ReadFen(unprotected);
	whiteScore = KingExposed(6, &g_mainGame);
	blackScore = KingExposed(62, &g_mainGame);
	AssertAreEqualInts(48, whiteScore, "Not the expected exposure score for white");
	AssertAreEqualInts(48, blackScore, "Not the expected exposure score for black");

}

void PassedPawnTest() {
	printf("%s\n", __func__);
	char* fen = "rnbqkbnr/3p3p/2p3p1/5p2/P7/1P5P/2PP4/RNBQKBNR w KQkq - 0 1";
	ReadFen(fen);
	short score = PassedPawn(24, &g_mainGame);
	AssertAreEqualInts(23, score, "Passed pawns score missmatch");
	
	score = PassedPawn(17, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(17, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(10, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(11, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(23, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	//black
	score = PassedPawn(42, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(51, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(37, &g_mainGame);
	AssertAreEqualInts(23, score, "Passed pawns score missmatch");

	score = PassedPawn(46, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	score = PassedPawn(55, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch");

	ReadFen("rnbqkbnr/7p/8/5p2/1pP1P3/7P/8/RNBQKBNR w KQkq - 0 1");
	score = PassedPawn(25, &g_mainGame);
	AssertAreEqualInts(23, score, "Passed pawns score missmatch  square 25");

	score = PassedPawn(26, &g_mainGame);
	AssertAreEqualInts(23, score, "Passed pawns score missmatch  square 36");

	score = PassedPawn(28, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch  square 28");

	score = PassedPawn(37, &g_mainGame);
	AssertAreEqualInts(0, score, "Passed pawns score missmatch square 37");

}


void indexOfTest() {
	char* s2 = "Kristian Ekman";
	Assert(IndexOf(s2, "Ekman") == 9, "index of failed");
}

void containsNotTest() {
	char* s2 = "Kristian Ekman";
	AssertNot(Contains(s2, "annika"), "containsNotTest failed");
}

void _runTests() {
	//BestMoveTest();
	printf("\nPress any key to continue.");
	int c = _getch();
	system("@cls||clear");
}

void runAllTests() {
	/*DoublePawnsTest();

	if (_failedAsserts == 0)
		PrintGreen("Success! Tests are good!\n");
	printf("Press any key to continue.\n");
	int c = _getch();
	return;*/
	
	_failedAsserts = 0;

#ifdef DEBUG
	EnpassantCollisionsTest();
	PerftSaveHashTest();
#endif // _DEBUG

	HashKeyTest();
	TimedTest(50000000, HashTablePerformance);
	PerftHashDbTest();
	HashTableRoundTrip();
	HashTableDepthTest();
	PerftTestStart();
	PerfTestPosition2();
	FenTest();
	FenTestHalfMoves();
	FenEnppasantTest();
	ValidMovesPromotionCaptureAndCastling();
	LongCastling();
	EnPassantFromFenTest();
	BlackCastlingRightsAfterKingMove();
	WhiteCastlingRightsAfterKingMove();
	EnPassantAfterMove();
	MaterialBlackPawnCapture();
	MaterialWhiteQueenCapture();
	MaterialPromotion();
	MaterialCaptureAndPromotion();
	EnPassantMaterial();
	//MobilityRookTest();
	DoublePawnsTest();
	KingExposureTest();
	//PassedPawnTest();
	/*PositionScorePawns();
	PositionScoreKnights();
	PositionScoreCastling();*/
	
	clock_t start = clock();

	BestMoveTestBlackCaptureBishop();
	TestWhiteMateIn2();
	BlackMatesIn5Deeping();
	BestMoveByWhite1();
	BestMoveByWhite3();
	BestMoveByBlack1();
	BestMoveByBlack4();
	BestMoveByBlack5();
	RookSacrificeByWhite();
	BlackMatesIn5a();
	WhiteMatesIn5b();
	WhiteMatesIn7();
	EngineMated();

	clock_t end = clock();
	float secs =(float) (end - start) / CLOCKS_PER_SEC;
	printf("Time: %.2fs\n", secs);	

	if (_failedAsserts == 0)
		PrintGreen("Success! Tests are good!\n");
	else
		PrintRed("There are failed tests.\n");

	printf("Press any key to continue.\n");
	_getch();
	system("@cls||clear");
}