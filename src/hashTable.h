#pragma once
#include <string.h>
#include "commons.h"

#define SLOTS 3

U64 StartHash;
U64 ZobritsPieceTypesSquares[23][64];
U64 ZobritsSides[2];
U64 ZobritsCastlingRights[4];
U64 ZobritsEnpassantFile[9];

typedef enum {
	EXACT,
	BEST_BLACK,
	BEST_WHITE
} HashEntryType;

typedef struct {
	int Key2;
	short Score;
	HashEntryType Type;
	char Depth;
	char From;
	char To;
} HashEntry;

typedef struct {
	U64 EntrySlots[SLOTS];
} EntrySlot;

typedef struct {
	uint EntryCount;
	EntrySlot* Entries;
} HashTable;


void AddHashScore(U64 hash, short score, char depth, HashEntryType type, char from, char to);
bool GetScoreFromHash(U64 hash, char depth, short* score, Move* pvMove, short best_black, short best_white);
bool GetBestMoveFromHash(U64 hash, Move* move);

void AllocateHashTable(uint megabytes);
void GenerateZobritsKeys();
void ClearHashTable();
bool GetBestMoveFromHash(U64 hash, Move* move);
void PrintHashStats();
uint HashFull();