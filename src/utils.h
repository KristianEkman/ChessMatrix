#pragma once
#include "commons.h"

int ParseChar(char c);

U64 Llrand();

int RandomInt(int lower, int upper);

bool Streq(char s1[], char s2[]);

bool StartsWith(char a[], char b[]);

void Stdout_wl(const char* text);

bool Contains(char a[], char b[]);

int IndexOf(char * a, char * b);

typedef enum {
	black,
	blue,
	green,
	marine,
	red,
	purple,
	lightbrown,
	lightgray,
	gray,
	lightblue,
	lightgreen,
	seagreen,
	orange,
	lightpurple,
	yellow,
	white,
} ConsoleColor;

void PrintRed(const char* msg);

void PrintGreen(const char* msg);
void PrintInverted(const char* msg);

void ColorPrint(const char* text, ConsoleColor textColor, ConsoleColor background);
