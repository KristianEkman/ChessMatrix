#pragma once

unsigned long long ZobritsPieceTypesSquares[16][64];
unsigned long long ZobritsSides[2];
unsigned long long ZobritsCastlingRights[4];

void addEntry(unsigned long long hash, short score, char depth);
short getScoreFromHash(unsigned long long hash);

void GenerateZobritsKeys();