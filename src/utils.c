#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include <string.h>



int parseChar(char c) {
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

U64 llrand() {
	U64 r = 0;

	for (int i = 0; i < 5; ++i) {
		r = (r << 15) | (rand() & 0x7FFF);
	}

	return r & 0xFFFFFFFFFFFFFFFFULL;
}


bool streq(char s1[], char s2[])
{
	return strcmp(s1, s2) == 0;
}

bool startsWith(char a[], char b[])
{
	if (strncmp(a, b, strlen(b)) == 0) return true;
	return false;
}

bool contains(char a[], char b[]) {
	return strstr(a, b) != NULL;	
}

int indexOf(char * a, char * b) {
	char * p = strstr(a, b);
	if (p == NULL) return -1;
	return p -  a;
}


void stdout_wl(char* text) {
	printf("%s\n", text);
	fflush(stdout);
}