#pragma once

#define IndexLength 0x1FFFFFF
#define SlotCount 4
//entry of 8 bytes gives xGB big array
//warning, to big Hash table will fail Debug mode and program will bahave strangly.

unsigned long long ZobritsPieceTypesSquares[23][64];
unsigned long long ZobritsSides[2];
unsigned long long ZobritsCastlingRights[4];
unsigned long long ZobritsEnpassantFile[9];
//
//int HashTableFullCount;
//int HashTableEntries;
//int HashTableMatches;

void addHashScore(unsigned long long hash, short score, char depth);
short getScoreFromHash(unsigned long long hash, bool * empty, int depth);

void GenerateZobritsKeys();
void ClearHashTable();