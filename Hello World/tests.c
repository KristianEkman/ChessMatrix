#include <stdio.h>
#include <time.h>
#include <string.h>
#include <Windows.h>

#include "main.h"

void printRed(char * msg) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	WORD saved_attributes;

	/* Save current attributes */
	GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
	saved_attributes = consoleInfo.wAttributes;

	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
	printf(msg);

	/* Restore original attributes */
	SetConsoleTextAttribute(hConsole, saved_attributes);
}

AssertAreEqual(char * s1, char * s2, char * msg) {
	if (strcmp(s1, s2))
	{
		printRed(msg);
		printf("\n");
	}
}

void PerftTest() {

	char * fen1 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq";
	ReadFen(fen1);
	PrintGame();

	for (size_t i = 0; i < 2; i++)
	{
		perftCount = 0;
		clock_t start = clock();
		Perft(4);
		clock_t stop = clock();
		float secs = (float)(stop - start) / CLOCKS_PER_SEC;
		printf("%.2fs\n", secs);
		printf("%dk moves\n", perftCount);
		printf("%.2fk moves/s\n", perftCount / (1000 * secs));

		PrintGame();
	}
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen1, outFen, "Failed PerftTest");
}

void FenTest() {
	char * fen1 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq";
	ReadFen(fen1);
	char outFen[100];
	WriteFen(outFen);
	AssertAreEqual(fen1, outFen, "Failed FenTest");
}

void runTests() {
	PerftTest();
	FenTest();

	printf("Press enter to continue.");
	getch();
	system("@cls||clear");
}


