#pragma once

#define IndexLength 0xFFFFFF
#define SlotCount 4
//entry of 8 bytes gives xGB big array
//warning, to big Hash table will fail Debug mode and program will bahave strangly.

U64 ZobritsPieceTypesSquares[23][64];
U64 ZobritsSides[2];
U64 ZobritsCastlingRights[4];
U64 ZobritsEnpassantFile[9];
//
//int HashTableFullCount;
//int HashTableEntries;
//int HashTableMatches;

void addHashScore(U64 hash, short score, char depth, HashEntryType type, char from, char to);
bool getScoreFromHash(U64 hash, char depth, short* score, char* from, char* to, short alpha, short beta);

void GenerateZobritsKeys();
void ClearHashTable();