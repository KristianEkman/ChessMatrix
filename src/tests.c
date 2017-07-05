#include <stdio.h>
#include <time.h>
#include <string.h>
#include <Windows.h>
#include <conio.h>

#include "main.h"
#include "basic_structs.h"
#include "HashTable.h"
#include "utils.h"
#pragma region TestsHelpers

int _failedAsserts = 0;

void printColor(char * msg, int color) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	WORD saved_attributes;

	/* Save current attributes */
	GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
	saved_attributes = consoleInfo.wAttributes;

	SetConsoleTextAttribute(hConsole, color | FOREGROUND_INTENSITY);
	printf(msg);

	/* Restore original attributes */
	SetConsoleTextAttribute(hConsole, saved_attributes);
}

void printRed(char * msg) {
	printColor(msg, FOREGROUND_RED);
}

void printGreen(char * msg) {
	printColor(msg, FOREGROUND_GREEN);
}

void Assert(int goodResult, char * msg) {
	if (goodResult == 0)
	{
		printf("\n");
		printRed(msg);
		printf("\n");
		_failedAsserts++;
	}
}

void AssertNot(int result, char * msg) {
	if (result != 0)
	{
		printf("\n");
		printRed(msg);
		printf("\n");
		_failedAsserts++;
	}
}

void AssertAreEqual(char * s1, char * s2, char * msg) {
	if (strcmp(s1, s2))
	{
		printf("\n");
		printRed(msg);
		printf("\n");
		printRed(s1);
		printf("\n");
		printRed(s2);
		_failedAsserts++;
	}
}

void AssertAreEqualInts(int expected, int actual, char * msg) {
	if (expected != actual)
	{
		printf("\n");
		printRed(msg);
		printf("\n");
		char str[24];
		snprintf(str, 24, "Expected %d", expected);
		printRed(str);
		printf("\n");
		snprintf(str, 24, "Actual   %d", actual);
		printRed(str);
		_failedAsserts++;
	}
}

void printPerftResults() {
	printf("\nCaptures: %d\nCastles: %d\nChecks Mates: %d\nChecks: %d\nEn passants: %d\nPromotions %d\n",
		game.PerftResult.Captures, game.PerftResult.Castles, game.PerftResult.CheckMates, game.PerftResult.Checks,
		game.PerftResult.Enpassants, game.PerftResult.Promotions);
}

void MoveToString(Move move, char * sMove) {
	char fromFile = (move.From & 7) + 'a';
	char fromRank = (move.From >> 3) + '1';
	char toFile = (move.To & 7) + 'a';
	char toRank = (move.To >> 3) + '1';
	sMove[0] = fromFile;
	sMove[1] = fromRank;
	sMove[2] = '-';
	sMove[3] = toFile;
	sMove[4] = toRank;
	sMove[5] = '\0';
}
#pragma endregion


int PerftTest(char * fen, int depth) {

	ReadFen(fen);
	//PrintGame();
	int perftCount = 0;
	for (size_t i = 0; i < 2; i++)
	{
		clock_t start = clock();
		game.PerftResult.Captures = 0;
		game.PerftResult.Castles = 0;
		game.PerftResult.CheckMates = 0;
		game.PerftResult.Checks = 0;
		game.PerftResult.Enpassants = 0;
		game.PerftResult.Promotions = 0;
		short startScore = TotalMaterial();
		perftCount = Perft(depth);
		AssertAreEqualInts(startScore, TotalMaterial(), "Game material missmatch");
		//printPerftResults();
		clock_t stop = clock();
		float secs = (float)(stop - start) / CLOCKS_PER_SEC;
		//printf("%.2fs\n", secs);
		//printf("%d moves\n", perftCount);
		printf("\n%.2fk moves/s\n", perftCount / (1000 * secs));

		//PrintGame();
	}
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen, outFen, "Start and end FEN differ");
	return perftCount;
}

void FenTest() {
	char * fen1 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
	ReadFen(fen1);
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen1, outFen, "Start and end fen differ");
}

void TimedTest(int iterations, void(*func)(int)) {
	clock_t start = clock();
	(*func)(iterations);
	clock_t stop = clock();

	float secs = (float)(stop - start) / CLOCKS_PER_SEC;
	
	printf("\n%.2fk iterations/s\n", iterations / (1000 * secs));

}

void PerfTestPosition2() {
	printf("\n");printf(__func__);
	char * fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
	PerftTest(fen, 4);
	AssertAreEqualInts(757163, game.PerftResult.Captures, "Captures missmatch");
	AssertAreEqualInts(128013, game.PerftResult.Castles, "Castles missmatch");
	AssertAreEqualInts(1929, game.PerftResult.Enpassants, "En passants missmatch");
	AssertAreEqualInts(15172, game.PerftResult.Promotions, "Promotion missmatch");
}

void PerftTestStart() {
	printf("\n");printf(__func__);
	char * startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
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
	printf("\n");printf(__func__);
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
	printf("\n");printf(__func__);
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
	printf("\n");printf(__func__);
	char * fen = "8/5k2/8/3Pp3/8/8/8/4K3 w - e6 0 3";
	ReadFen(fen);
	Move moves[100];
	int count = ValidMoves(moves);
	Move expectedMove = parseMove("d5-e6", EnPassantCapture);
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
	int startGameScore = TotalMaterial();
	AssertNot(MakePlayerMove("d5-e6").Invalid, "Invalid move");
	AssertAreEqualInts(startGameScore - 100, TotalMaterial(), "Material should decrease by 100");
}

void EnPassantAfterMove() {
	printf("\n");printf(__func__);
	char * fen = "4k3/4p3/8/3P4/8/8/8/4K3 b - e3 0 1";
	ReadFen(fen);
	AssertNot(MakePlayerMove("e7-e5").Invalid, "Move was not valid");

	Move moves[100];
	int count = ValidMoves(moves);
	Move expectedMove = parseMove("d5-e6", EnPassantCapture);
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
}

void MaterialBlackPawnCapture() {
	printf("\n");printf(__func__);
	ReadFen("2r1k3/8/8/4p3/3P4/8/8/2Q1K3 w - - 0 1");
	AssertAreEqualInts(-400, TotalMaterial(), "Start Material missmatch");
	AssertNot(MakePlayerMove("d4-e5").Invalid, "Move was not valid");
	AssertAreEqualInts(-500, TotalMaterial(), "Game Material missmatch");
}

void MaterialWhiteQueenCapture() {
	printf("\n");printf(__func__);
	ReadFen("rnbqkbnr/ppp1pppp/8/3p4/4Q3/4P3/PPPP1PPP/RNB1KBNR b KQkq - 0 1");
	AssertAreEqualInts(0, TotalMaterial(), "Start Material missmatch");
	AssertNot(MakePlayerMove("d5-e4").Invalid, "Move was not valid");
	AssertAreEqualInts(900, TotalMaterial(), "Game Material missmatch");
}

void MaterialCaptureAndPromotion() {
	printf("\n");printf(__func__);
	ReadFen("2r1k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
	AssertAreEqualInts(400, TotalMaterial(), "Start Material missmatch");
	PlayerMove pm = MakePlayerMove("b7-c8");
	AssertNot(pm.Invalid, "Move was not valid");
	AssertAreEqualInts(-900, TotalMaterial(), "Game Material missmatch");
	UnMakePlayerMove(pm);
	AssertAreEqualInts(400, TotalMaterial(), "Start Material missmatch");
}

void MaterialPromotion() {
	printf("\n");printf(__func__);
	ReadFen("2r1k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
	AssertAreEqualInts(400, TotalMaterial(), "Start Material missmatch");
	AssertNot(MakePlayerMove("b7-b8").Invalid, "Move was not valid");
	AssertAreEqualInts(-400, TotalMaterial(), "Game Material missmatch");
}

void HashTableRoundTrip() {
	printf("\n");printf(__func__);
	unsigned long long hash = 0x1234567890ABCDEF;
	short expected = 3000;
	addEntry(hash, expected);
	short score = getScoreFromHash(hash);
	AssertAreEqualInts(expected, score, "hash table score missmatch");

	unsigned long long hash2 = hash + 1;
	short expected2 = 4000;
	addEntry(hash2, expected2);
	short score2 = getScoreFromHash(hash2);
	AssertAreEqualInts(expected2, score2, "hash table score missmatch");

	score = getScoreFromHash(hash);
	AssertAreEqualInts(expected, score, "hash table score missmatch");
}

void HashTablePerformance(int iterations) {
	printf("\n");printf(__func__);
	unsigned long long hash = llrand();
	short expected = 1;

	for (int i = 0; i < iterations; i++)
	{
		expected++;
		hash ++;
		addEntry(hash, expected);
		short score = getScoreFromHash(hash);
		AssertAreEqualInts(expected, score, "hash table score missmatch");
	}
}

void EnPassantMaterial() {
	printf("\n");printf(__func__);
	ReadFen("r3k3/3p4/8/4P3/8/8/8/4K2R b - - 0 1");
	AssertAreEqualInts(0, TotalMaterial(), "Start Material missmatch");
	AssertNot(MakePlayerMove("d7-d5").Invalid, "Move was not valid");
	PlayerMove nextMove = MakePlayerMove("e5-d6");
	AssertNot(nextMove.Invalid, "Move was not valid");
	AssertAreEqualInts(-100, TotalMaterial(), "Game Material missmatch");
	UnMakePlayerMove(nextMove);
	AssertAreEqualInts(0, TotalMaterial(), "Game Material missmatch");
}

void PositionScorePawns() {
	printf("\n");printf(__func__);
	char * startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
	ReadFen(startFen);
	AssertAreEqualInts(0, game.PositionScore, "Position Score Missmatch");
	MakePlayerMove("d2-d4");
	AssertAreEqualInts(-40, game.PositionScore, "Position Score Missmatch");
	PlayerMove m2 = MakePlayerMove("d7-d5");
	AssertAreEqualInts(0, game.PositionScore, "Position Score Missmatch");
	UnMakePlayerMove(m2);
	AssertAreEqualInts(-40, game.PositionScore, "Position Score Missmatch");
}

void PositionScoreKnights() {
	printf("\n");printf(__func__);
	char * startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
	ReadFen(startFen);
	MakePlayerMove("b1-c3");
	AssertAreEqualInts(-50, game.PositionScore, "Position Score Missmatch");
	PlayerMove m2 = MakePlayerMove("g8-f6");
	AssertAreEqualInts(0, game.PositionScore, "Position Score Missmatch");
	UnMakePlayerMove(m2);
	AssertAreEqualInts(-50, game.PositionScore, "Position Score Missmatch");
}

void PositionScoreCastling() {
	printf("\n");printf(__func__);
	char * startFen = "r3k2r/p1qnbppp/bpp2n2/3pp3/B2P4/2N1PN2/PPPBQPPP/R3K2R w KQkq - 0 1";
	ReadFen(startFen);
	AssertAreEqualInts(-15, game.PositionScore, "Position Score Missmatch");

	MakePlayerMove("e1-g1");
	AssertAreEqualInts(-45, game.PositionScore, "Position Score Missmatch");

	MakePlayerMove("e8-g8");
	AssertAreEqualInts(-15, game.PositionScore, "Position Score Missmatch");
}

void BestMoveTest() {
	printf("\n");printf(__func__);
	char * startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
	ReadFen(startFen);

	clock_t start = clock();
	Move bestMove = BestMoveAtDepth(5);
	clock_t stop = clock();

	float secs = (float)(stop - start) / CLOCKS_PER_SEC;
	printf("\nFrom:%u  To:%u  Leafs:%u", bestMove.From, bestMove.To, SearchedLeafs);
	printf("\n%.2fk leafs/s\n", SearchedLeafs / (1000 * secs));
}


void BestMoveTestBlackCaptureBishop() {
	printf("\n");printf(__func__);
	char * startFen = "r1bqk2r/ppp1bppp/2n1pn2/3p4/2BP1B2/2N1PN2/PPP2PPP/R2QK2R b KQkq - 2 6";
	ReadFen(startFen);

	Move bestMove = BestMoveAtDepth(4);
	char sMove[5];
	MoveToString(bestMove, sMove);
	AssertAreEqual("d5-c4", sMove, "Not the expected move");
}

void TestWhiteMateIn2() {
	printf("\n");printf(__func__);
	char * startFen = "5k2/8/2Q5/3R4/8/8/8/4K3 w - - 2 1";
	ReadFen(startFen);

	Move bestMove = BestMoveAtDepth(2);
	char sMove[6];
	MoveToString(bestMove, sMove);
	AssertAreEqual("d5-d7", sMove, "Not the expected move");
}

//mate in 7 2r1nrk1/p4p1p/1p2p1pQ/nPqbRN2/8/P2B4/1BP2PPP/3R2K1 w - - 0 1
//f5-e7

void TestBlackMateIn5() {
	printf("\n");printf(__func__);
	char * startFen = "1k2r3/pP3pp1/8/3P1B1p/5q2/N1P2b2/PP3Pp1/R5K1 b - - 0 1";
	ReadFen(startFen);

	Move bestMove = BestMoveAtDepth(5);
	char sMove[6];
	MoveToString(bestMove, sMove);
	AssertAreEqual("f4-h4", sMove, "Not the expected move");
}

void runTests() {
	_failedAsserts = 0;
	/*PerftTestStart();
	PerfTestPosition2();
	FenTest();
	ValidMovesPromotionCaptureAndCastling();
	LongCastling();
	EnPassantFromFenTest();
	EnPassantAfterMove();
	MaterialBlackPawnCapture();
	MaterialWhiteQueenCapture();
	MaterialPromotion();
	MaterialCaptureAndPromotion();
	EnPassantMaterial();
	HashTableRoundTrip();
	TimedTest(100000000, HashTablePerformance);
	PositionScorePawns();
	PositionScoreKnights();
	PositionScoreCastling();
	BestMoveTest();
	BestMoveTestBlackCaptureBishop();
	TestWhiteMateIn2();*/
	TestBlackMateIn5();
	if (_failedAsserts == 0)
		printGreen("\nSuccess! Tests are good!");

	printf("\nPress any key to continue.");
	_getch();
	system("@cls||clear");
}


