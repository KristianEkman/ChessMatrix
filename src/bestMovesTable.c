#include "basic_structs.h"


typedef struct {
	unsigned long long PositionHash;
	char from;
	char to;
} BestMovesTableEntry;

typedef struct
{
	BestMovesTableEntry* Entries;
	int NumberOfEntries;
} BestMovesTable;

BestMovesTable * bmTable;

void InitBestMovesTable(int megabytes) {

}

void AddBestMovesEntry(unsigned long long hash, char from, char to) {

}

Move GetBestMove(unsigned long long hash) {
	Move move;

	return move;
}