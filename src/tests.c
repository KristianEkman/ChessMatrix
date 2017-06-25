#include <stdio.h>
#include <time.h>
#include <string.h>
#include <Windows.h>
#include <conio.h>


#include "main.h"
#include "basic_structs.h"
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
		_perftResult.Captures, _perftResult.Castles, _perftResult.CheckMates, _perftResult.Checks,
		_perftResult.Enpassants, _perftResult.Promotions);
}

void printMoves(int count, Move * moves) {
	for (int i = 0; i < count; i++)
	{
		char sMove[6];
		sMove[0] = (moves[i].From & 7) + 'a';
		sMove[1] = (moves[i].From >> 3) + '1';
		sMove[2] = '-';
		sMove[3] = (moves[i].To & 7) + 'a';
		sMove[4] = (moves[i].To >> 3) + '1';
		sMove[5] = '\0';

		printf("%s, ", sMove);
	}
}

int PerftTest(char * fen, int depth) {

	ReadFen(fen);
	//PrintGame();
	int perftCount = 0;
	for (size_t i = 0; i < 2; i++)
	{
		clock_t start = clock();
		_perftResult.Captures = 0;
		_perftResult.Castles = 0;
		_perftResult.CheckMates = 0;
		_perftResult.Checks = 0;
		_perftResult.Enpassants = 0;
		_perftResult.Promotions = 0;
		short startScore = GameMaterial;
		perftCount = Perft(depth);
		AssertAreEqualInts(startScore, GameMaterial, "Game material missmatch");
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

#pragma region Tests

void PerfTestPosition2() {
	printf("\n");printf(__func__);
	char * fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
	PerftTest(fen, 4);
	AssertAreEqualInts(757163, _perftResult.Captures, "Captures missmatch");
	AssertAreEqualInts(128013, _perftResult.Castles,  "Castles missmatch");
	AssertAreEqualInts(1929, _perftResult.Enpassants,  "En passants missmatch");
	AssertAreEqualInts(15172, _perftResult.Promotions, "Promotion missmatch");
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
	//printMoves(count, moves);
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
	int startGameScore = GameMaterial;
	Assert(MakePlayerMove("d5-e6"), "Invalid move");
	AssertAreEqualInts(startGameScore - 100, GameMaterial, "Material should decrease by 100");
}

void EnPassantAfterMove() {
	printf("\n");printf(__func__);
	char * fen = "4k3/4p3/8/3P4/8/8/8/4K3 b - e3 0 1";
	ReadFen(fen);
	Assert(MakePlayerMove("e7-e5"), "Move was not valid");

	Move moves[100];
	int count = ValidMoves(moves);
	Move expectedMove = parseMove("d5-e6", EnPassantCapture);
	Assert(MovesContains(moves, count, expectedMove), "The move was not found");
}

void MaterialBlackPawnCapture() {
	printf("\n");printf(__func__);
	ReadFen("2r1k3/8/8/4p3/3P4/8/8/2Q1K3 w - - 0 1");
	AssertAreEqualInts(-400, GameMaterial, "Start Material missmatch");
	Assert(MakePlayerMove("d4-e5"), "Move was not valid");
	AssertAreEqualInts(-500, GameMaterial, "Game Material missmatch");
}

void MaterialWhiteQueenCapture() {
	printf("\n");printf(__func__);
	ReadFen("rnbqkbnr/ppp1pppp/8/3p4/4Q3/4P3/PPPP1PPP/RNB1KBNR b KQkq - 0 1");
	AssertAreEqualInts(0, GameMaterial, "Start Material missmatch");
	Assert(MakePlayerMove("d5-e4"), "Move was not valid");
	AssertAreEqualInts(900, GameMaterial, "Game Material missmatch");
}

void MaterialCaptureAndPromotion() {
	printf("\n");printf(__func__);
	ReadFen("2r1k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
	AssertAreEqualInts(400, GameMaterial, "Start Material missmatch");
	Assert(MakePlayerMove("b7-c8"), "Move was not valid");
	AssertAreEqualInts(-900, GameMaterial, "Game Material missmatch");	
}

void MaterialPromotion() {
	printf("\n");printf(__func__);
	ReadFen("2r1k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
	AssertAreEqualInts(400, GameMaterial, "Start Material missmatch");
	Assert(MakePlayerMove("b7-b8"), "Move was not valid");
	AssertAreEqualInts(-400, GameMaterial, "Game Material missmatch");
}

#pragma endregion

void runTests() {
	_failedAsserts = 0;
	PerftTestStart();
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
	if (_failedAsserts == 0)
		printGreen("\nSuccess! Tests are good!");

	printf("\nPress any key to continue.");
	_getch();
	system("@cls||clear");
}


