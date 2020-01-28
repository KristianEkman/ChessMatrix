#pragma once
#include <stdbool.h>
#include "commons.h"

int parseChar(char c);

U64 llrand();

bool streq(char s1[], char s2[]);

bool startsWith(char a[], char b[]);

void stdout_wl(char* text);

bool contains(char a[], char b[]);

int indexOf(char * a, char * b);