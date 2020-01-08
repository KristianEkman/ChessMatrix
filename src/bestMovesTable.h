//#pragma once
//
//typedef struct {
//	unsigned long long PositionHash;
//	char from;
//	char to;
//} BestMovesTableEntry;
//
//typedef struct
//{
//	BestMovesTableEntry * pEntries;
//	unsigned int NumberOfEntries;
//} BestMovesTable;
//
//void ClearTable(BestMovesTable * bmTable);
//
//void InitBestMovesTable(BestMovesTable* bmTable, int sizeMb);
//
//void AddBestMovesEntry(BestMovesTable* bmTable, unsigned long long hash, char from, char to);
//
//Move GetBestMove(BestMovesTable* bmTable, unsigned long long hash);
//
