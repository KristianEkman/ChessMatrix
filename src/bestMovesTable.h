#pragma once

typedef struct {
	unsigned long long PositionHash;
	char from;
	char to;
} BestMovesTableEntry;

typedef struct
{
	BestMovesTableEntry pEntries[32000000];
	int NumberOfEntries;
} BestMovesTable;

void ClearTable();

void InitBestMovesTable();

void AddBestMovesEntry(unsigned long long hash, char from, char to);

Move GetBestMove(unsigned long long hash);