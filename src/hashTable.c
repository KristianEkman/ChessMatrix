#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <Windows.h>

#include "basic_structs.h"
#include "utils.h"
#include "hashTable.h"

//		bits shift >>   mask
//key2	31	 0	        7FFF FFFF
//score	14	 31	        3FFF
//depth	5	 45	        1F
//type	2	 50	        3
//from	6	 52	        3F
//to	6	 58	        3F

HashTable H_Table;
void addHashScore(U64 hash, short score, char depth, HashEntryType type, char from, char to) {
	unsigned int key2 = hash & 0x7FFFFFFF;
	U64 pack = key2 & 0x7FFFFFFF;
	pack |= (((U64)score + MAX_SCORE) << 31); //Make sure it is positive by adding max score.
	pack |= (U64)depth << 45;
	pack |= (U64)type << 50;
	pack |= (U64)from << 52;
	pack |= (U64)to << 58;

	unsigned int idx = (unsigned int)(hash % H_Table.EntryCount);
	U64 entry = H_Table.Entries[idx];
	int dbDepth = (entry >> 45) & 0x1F;
	unsigned int dbKey2 = entry & 0x7FFFFFFF;

	//always overwrite unless same hash with lower depth is stored.
	//note: a new hash but with colliding index will overwrite previous. Could be prevented by a few "Slots" per index.
	if (dbKey2 == key2) {
		if (depth < dbDepth)
			return;
	}

	
	H_Table.Entries[idx] = pack;
	//HashTableFullCount++;
}

bool getScoreFromHash(U64 hash, char depth, short* score, Move* pvMove, short alpha, short beta) {
	unsigned int idx = (unsigned int)(hash % H_Table.EntryCount);
	unsigned int key2 = hash & 0x7FFFFFFF;

	U64 entry = H_Table.Entries[idx];
	unsigned int dbKey = entry & 0x7FFFFFFF;
	int dbDepth = (entry >> 45) & 0x1F;
	if (dbKey == key2) {
		pvMove->From = (entry >> 52) & 0x3F;
		pvMove->To = (entry >> 58) & 0x3F;
		pvMove->MoveInfo = PlainMove;
		if (dbDepth >= depth)
		{
			*score = ((entry >> 31) & 0x3FFF) - MAX_SCORE;
			//todo: adjust mate scores with in_deep

			HashEntryType type = (entry >> 50) & 0x3;
			switch (type)
			{
			case ALPHA:
				if (*score <= alpha) { // is this true for both black and white?
					*score = alpha;
					return true;
				}
				break;
			case BETA:
				if (*score >= beta) { // is this true for both black and white?
					*score = beta;
					return true;
				}
				break;
			case EXACT:
				return true;
				break;
			default:
				break;
			}
		}
	}
	//HashTableMatches++;
	return false;
}

bool getBestMoveFromHash(U64 hash, Move* move) {
	unsigned int idx = (unsigned int)(hash % H_Table.EntryCount);
	unsigned int key2 = hash & 0x7FFFFFFF;

	U64 entry = H_Table.Entries[idx];
	unsigned int dbKey = entry & 0x7FFFFFFF;
	int dbDepth = (entry >> 45) & 0x1F;
	if (dbKey == key2) {
		move->From = (entry >> 52) & 0x3F;
		move->To = (entry >> 58) & 0x3F;
		return true;
	}
	return false;
}

void GenerateZobritsKeys() {
	for (int i = 0; i < 23; i++) //only using 16 of these, but King | BLACK is 22 and some are not use. See also PieceType enum.
		for (int s = 0; s < 64; s++)
			ZobritsPieceTypesSquares[i][s] = llrand();
	//Setting nopiece to zeros. Will not affect hash.
	for (int i = 0; i < 64; i++)
		ZobritsPieceTypesSquares[0][i] = 0;

	ZobritsSides[0] = llrand();
	ZobritsSides[1] = llrand();
	for (int i = 0; i < 4; i++)
		ZobritsCastlingRights[i] = llrand();

	ZobritsEnpassantFile[0] = 0; //no enpassant file
	for (int i = 1; i < 9; i++)
		ZobritsEnpassantFile[i] = llrand();
}

void Allocate(unsigned int megabytes) {
	free(H_Table.Entries);
	H_Table.EntryCount = (megabytes * 0x100000) / sizeof(U64);
	H_Table.Entries = malloc(H_Table.EntryCount * sizeof(U64));
}

void ClearHashTable() {
	for (size_t i = 0; i < H_Table.EntryCount; i++)
		H_Table.Entries[i] = 0ULL;
	/*HashTableFullCount = 0;
	HashTableEntries = 0;
	HashTableMatches = 0;*/
}
