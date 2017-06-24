#include <stdio.h>
#include <time.h>
#include <string.h>
#include <Windows.h>
#include <conio.h>


#include "main.h"
#include "basic_structs.h"


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

Move parseMove(char * sMove, MoveInfo info) {
	int fromFile = sMove[0] - 'a';
	int fromRank = sMove[1] - '1';
	int toFile = sMove[3] - 'a';
	int toRank = sMove[4] - '1';
	Move move;
	move.From = fromRank * 8 + fromFile;
	move.To = toRank * 8 + toFile;
	move.MoveInfo = info;
	return move;
}


void Assert(int goodResult, char * name, char * msg) {
	if (goodResult == 0)
	{
		printRed(name);
		printRed(": ");
		printRed(msg);
		printf("\n");
	}
	else {
		printf("\n");
		printGreen(name);
		printGreen(" succeded\n");
	}
}

void AssertAreEqual(char * s1, char * s2, char * name, char * msg) {
	if (strcmp(s1, s2))
	{
		printf("\n");
		printRed(name);
		printRed(": ");
		printRed(msg);
		printf("\n");
		printRed(s1);
		printf("\n");
		printRed(s2);
	}
	else {
		printf("\n");
		printGreen(name);
		printGreen(" succeded\n");
	}
}

void AssertAreEqualInts(int expected, int actual, char * name, char * msg) {
	if (expected != actual)
	{
		printf("\n");
		printRed(name);
		printRed(": ");
		printRed(msg);
		printf("\n");
		char str[24];
		snprintf(str, 24, "Expected %d", expected);
		printRed(str);
		printf("\n");
		snprintf(str, 24, "Actual   %d", actual);
		printRed(str);
	}
	else {
		printf("\n");
		printGreen(name);
		printGreen(" succeded\n");
	}
}

void printPerftResults() {
	printf("\nCaptures: %d\nCastles: %d\nChecks Mates: %d\nChecks: %d\nEn passants: %d\nPromotions %d\n", 
    _perftResult.Captures, _perftResult.Castles, _perftResult.CheckMates, _perftResult.Checks, 
		_perftResult.Enpassants, _perftResult.Promotions);
}

int PerftTest(char * fen1, int depth, char * name) {

	ReadFen(fen1);
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
		perftCount = Perft(depth);
		printPerftResults();
		clock_t stop = clock();
		float secs = (float)(stop - start) / CLOCKS_PER_SEC;
		printf("%.2fs\n", secs);
		printf("%d moves\n", perftCount);
		printf("%.2fk moves/s\n", perftCount / (1000 * secs));

		//PrintGame();
	}
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen1, outFen, name, "Start and end FEN differ");
	return perftCount;
}

void FenTest() {
	char * fen1 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
	ReadFen(fen1);
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen1, outFen, "Fen test", "Start and end fen differ");
}

void PerfTestComplexPosition() {
	char * fen1 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
	PerftTest(fen1, 4, __func__);
}

void PerftTestStart() {
	char * startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
	int count = PerftTest(startFen, 5, __func__);
	AssertAreEqualInts(4865609, count, "PerftTestStart", "Perft Count missmatch");

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

bool ArrayContains(Move * moves, int count, Move move) {
	for (int i = 0; i < count; i++)
	{
		if (moves[i].From == move.From && moves[i].To == move.To && moves[i].MoveInfo == move.MoveInfo) {
			return true;
		}
	}
	return false;
}

void ValidMovesPromotionCaptureAndCastling() {
	char * fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
	Move moves[100];
	ReadFen(fen);
	int count = ValidMoves(moves);
	printMoves(count, moves);
	AssertAreEqualInts(44, count, __func__, "Moves count missmatch");
	Move expectedMove;
	expectedMove.From = 4;
	expectedMove.To = 6;
	expectedMove.MoveInfo = CastleShort;
	Assert(ArrayContains(moves, count, expectedMove), __func__, "The move was not found");
}

void LongCastling() {
	char * fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
	Move moves[100];
	ReadFen(fen);
	int count = ValidMoves(moves);
	AssertAreEqualInts(48, count, __func__, "Moves count missmatch");
	Move expectedMove;
	expectedMove.From = 4;
	expectedMove.To = 2;
	expectedMove.MoveInfo = CastleLong;
	Assert(ArrayContains(moves, count, expectedMove), __func__, "The move was not found");
}

void EnPassantFromFenTest() {
	char * fen = "8/5k2/8/3Pp3/8/8/8/4K3 w - e6 0 3";
	ReadFen(fen);
	Move moves[100];
	int count = ValidMoves(moves);
	//todo check that the move exists
	Move expectedMove = parseMove("d5-e6", EnPassantCapture);
	Assert(ArrayContains(moves, count, expectedMove), __func__, "The move was not found");
}

void EnPassantAfterMove() {
	char * fen = "4k3/4p3/8/3P4/8/8/8/4K3 b - e3 0 1";
	ReadFen(fen);
	Move move = parseMove("e7-e5", PlainMove);
	Assert(MakePlayerMove(move), __func__, "Move was not valid");

	Move moves[100];
	int count = ValidMoves(moves);
	Move expectedMove = parseMove("d5-e6", EnPassantCapture);
	Assert(ArrayContains(moves, count, expectedMove), __func__, "The move was not found");
}

void runTests() {
	PerfTestComplexPosition();
	PerftTestStart();
	FenTest();
	ValidMovesPromotionCaptureAndCastling();
	LongCastling();
	EnPassantFromFenTest();
	EnPassantAfterMove();

	printf("\nPress any key to continue.");
	_getch();
	system("@cls||clear");
}


