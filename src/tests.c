#include <stdio.h>
#include <time.h>
#include <string.h>
#include <Windows.h>

#include "main.h"


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

void AssertAreEqual(char * s1, char * s2, char * name, char * msg) {
	if (strcmp(s1, s2))
	{
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

int PerftTest(char * fen1, int depth) {

	ReadFen(fen1);
	//PrintGame();
	int perftCount = 0;
	for (size_t i = 0; i < 2; i++)
	{
		clock_t start = clock();
		perftCount = Perft(depth);
		clock_t stop = clock();
		float secs = (float)(stop - start) / CLOCKS_PER_SEC;
		printf("%.2fs\n", secs);
		printf("%d moves\n", perftCount);
		printf("%.2fk moves/s\n", perftCount / (1000 * secs));

		//PrintGame();
	}
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen1, outFen, "Perft", "Start and end fen differ");
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
	PerftTest(fen1, 4);
}

void PerftTestStart() {
	char * startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
	int count = PerftTest(startFen, 5);
	AssertAreEqualInts(4865609, count, "PerftTestStart", "Perft Count missmatch");

}

void runTests() {
	PerfTestComplexPosition();
	PerftTestStart();
	FenTest();
	printf("Press enter to continue.");
	getch();
	system("@cls||clear");
}


