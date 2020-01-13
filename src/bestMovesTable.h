//#pragma once
//
//typedef struct {
//	U64 PositionHash;
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
//void AddBestMovesEntry(BestMovesTable* bmTable, U64 hash, char from, char to);
//
//Move GetBestMove(BestMovesTable* bmTable, U64 hash);
//
