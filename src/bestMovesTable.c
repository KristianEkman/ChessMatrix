#include<stdlib.h>

#include "basic_structs.h"
#include "bestMovesTable.h"

void ClearTable(BestMovesTable * bmTable) {
	/*for (BestMovesTableEntry * p = bmTable->pEntries; p < bmTable->pEntries + bmTable->NumberOfEntries; p++)
	{
		p->from = (char)0;
		p->to = (char)0;
		p->PositionHash = (unsigned long long)0;
	}*/
	for (int i = 0; i < bmTable->NumberOfEntries; i++)
	{
		bmTable->pEntries[i].from = (char)0;
		bmTable->pEntries[i].to = (char)0;
		bmTable->pEntries[i].PositionHash = (unsigned long long)0;
	}
}

void InitBestMovesTable(BestMovesTable* bmTable, int sizeMb) {
	bmTable->NumberOfEntries = ((sizeMb * 0x100000) / sizeof(BestMovesTableEntry)) - 2;
	bmTable->pEntries = (BestMovesTableEntry*)malloc(bmTable->NumberOfEntries * sizeof(BestMovesTableEntry));
	ClearTable(bmTable);
}

void AddBestMovesEntry(BestMovesTable* bmTable, unsigned long long hash, char from, char to) {
	int index = hash % bmTable->NumberOfEntries;
	bmTable->pEntries[index].PositionHash = hash;
	bmTable->pEntries[index].from = from;
	bmTable->pEntries[index].to = to;
}

Move GetBestMove(BestMovesTable* bmTable, unsigned long long hash) {
	int index = hash % bmTable->NumberOfEntries;
	Move move;
	move.From = 0;
	move.To = 0;
	move.MoveInfo = NotAMove;
	if (bmTable->pEntries[index].PositionHash == hash) {
		move.From = bmTable->pEntries[index].from;
		move.To = bmTable->pEntries[index].to;
		move.MoveInfo = PlainMove; // todo: Maybe no need to get correct move info here. IT will be decided later probably?
	}

	return move;
}