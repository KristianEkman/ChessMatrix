#pragma once
#include "moves.h"

typedef struct {
	char from;      /* a1 0, b1 1, ..., h1 7, ... h8 63 */
	char to;        /* a1 0, b1 1, ..., h1 7, ... h8 63 */
	char promotion; /* 0 none, +-1 rook, +-2 knight, +-3 bishop, +-4 queen */
	char priority;
	int ngames;
	int nwon;
	int nlost;
	int plycount;
	int nextMove;
	int nextSibling;
} BookEntry;

typedef struct {
	U64 positionHash;
	BookEntry bookEntry;
} HashBookEntry;

void loadBook(char* fileName);
void closeBook();
Move bestBookMove(Game* game);
bool OwnBook;