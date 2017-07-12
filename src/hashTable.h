#pragma once

#define IndexLength 0xFFFFFF
#define SlotCount 3

unsigned long long ZobritsPieceTypesSquares[23][64];
unsigned long long ZobritsSides[2];
unsigned long long ZobritsCastlingRights[4];
unsigned long long ZobritsEnpassantFile[9];


void addEntry(unsigned long long hash, short score, char depth);
short getScoreFromHash(unsigned long long hash);

void GenerateZobritsKeys();
void ClearHashTable();