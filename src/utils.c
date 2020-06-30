#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include <string.h>

#include <Windows.h>
#include <conio.h>



int ParseChar(char c) {
	switch (c)
	{
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;
	default:
		return 0;
	}
}

U64  rnd_seed = 1070372;

U64 _llrand() {

	rnd_seed ^= rnd_seed >> 12, rnd_seed ^= rnd_seed << 25, rnd_seed ^= rnd_seed >> 27;
	return rnd_seed * 2685821657736338717LL;
}

U64 Llrand() {
	U64 r = 0;

	for (int i = 0; i < 5; ++i) {
		r = (r << 15) | (rand() & 0x7FFF);
	}

	return r & 0xFFFFFFFFFFFFFFFFULL;
}


bool Streq(char s1[], char s2[])
{
	return strcmp(s1, s2) == 0;
}

bool StartsWith(char a[], char b[])
{
	if (strncmp(a, b, strlen(b)) == 0) return true;
	return false;
}

bool Contains(char a[], char b[]) {
	return strstr(a, b) != NULL;	
}

int IndexOf(char * a, char * b) {
	char * p = strstr(a, b);
	if (p == NULL) return -1;
	return p -  a;
}


void Stdout_wl(char* text) {
	printf("%s\n", text);
	fflush(stdout);
}

void printColor(char* msg, int color) {
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

void PrintRed(char* msg) {
	printColor(msg, FOREGROUND_RED);
}

void PrintGreen(char* msg) {
	printColor(msg, FOREGROUND_GREEN);
}
