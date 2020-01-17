#pragma once

U64 ZobritsPieceTypesSquares[23][64];
U64 ZobritsSides[2];
U64 ZobritsCastlingRights[4];
U64 ZobritsEnpassantFile[9];
//
//int HashTableFullCount;
//int HashTableEntries;
//int HashTableMatches;


typedef enum HashEntryType {
	EXACT,
	ALPHA,
	BETA
} HashEntryType;

typedef struct HashEntry {
	int Key2;
	short Score;
	HashEntryType Type;
	char Depth;
	char From;
	char To;
} HashEntry;

typedef struct HashTable {
	unsigned int EntryCount;
	U64* Entries;
} HashTable;

void addHashScore(U64 hash, short score, char depth, HashEntryType type, char from, char to);
bool getScoreFromHash(U64 hash, char depth, short* score, Move* pvMove, short alpha, short beta);
bool getBestMoveFromHash(U64 hash, Move* move);

void Allocate(unsigned int megabytes);
void GenerateZobritsKeys();
void ClearHashTable();
bool getBestMoveFromHash(U64 hash, Move* move);



