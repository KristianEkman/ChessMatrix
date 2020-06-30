#pragma once
#include "commons.h"

int ParseChar(char c);

U64 Llrand();

bool Streq(char s1[], char s2[]);

bool StartsWith(char a[], char b[]);

void Stdout_wl(char* text);

bool Contains(char a[], char b[]);

int IndexOf(char * a, char * b);

void PrintRed(char* msg);

void PrintGreen(char* msg);
