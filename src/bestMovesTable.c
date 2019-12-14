#include "basic_structs.h"
#include "bestMovesTable.h"

//const int BestMoveTableSize = 0x100000 * 2;

BestMovesTable bmTable;

void ClearTable() {
	for (int i = 0; i < bmTable.NumberOfEntries; i++)
	{
		bmTable.pEntries[i].from = (char)0;
		bmTable.pEntries[i].to = (char)0;
		bmTable.pEntries[i].PositionHash = (unsigned long long)0;
	}
}

void InitBestMovesTable() {
	bmTable.NumberOfEntries = 32000000;
	ClearTable();
}

void AddBestMovesEntry(unsigned long long hash, char from, char to) {
	int index = hash % bmTable.NumberOfEntries;
	bmTable.pEntries[index].PositionHash = hash;
	bmTable.pEntries[index].from = from;
	bmTable.pEntries[index].to = to;
}

Move GetBestMove(unsigned long long hash) {
	int index = hash % bmTable.NumberOfEntries;
	Move move;
	move.From = 0;
	move.To = 0;
	move.MoveInfo = NotAMove;
	if (bmTable.pEntries[index].PositionHash == hash) {
		move.From = bmTable.pEntries[index].from;
		move.To = bmTable.pEntries[index].to;
		move.MoveInfo = PlainMove; // todo: Maybe no need to get correct move info here. IT will be decided later probably?
	}

	return move;
}