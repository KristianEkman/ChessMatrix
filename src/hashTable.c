#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "commons.h"
#include "utils.h"
#include "hashTable.h"

//		bits shift >>   mask
//key2	28	 0	        0FFF FFFF
//score	14	 28	        3FFF
//depth	5	 42	        1F
//type	2	 47	        3
//from	6	 49	        3F
//to	6	 55	        3F
//promo	3	 61	        7

HashTable H_Table;
int HashTableEntries;
int HashTableOverWrites;

void AddHashScore(U64 hash, short score, char depth, HashEntryType type, Move move) {
	uint key2 = hash & 0x0FFFFFFF; // Keep 28 bits for verification key to make room for promotion info.
	U64 pack = key2 & 0x0FFFFFFF;
	pack |= (((U64)score + MAX_SCORE) << 28); // Make sure it is positive by adding max score.
	pack |= (U64)depth << 42;
	pack |= (U64)type << 47;
	pack |= (U64)move.From << 49;
	pack |= (U64)move.To << 55;

	char promo = 0;
	if (move.MoveInfo >= PromotionQueen && move.MoveInfo <= PromotionKnight) {
		promo = move.MoveInfo;
	}
	pack |= (U64)promo << 61;

	uint idx = (uint)(hash % H_Table.EntryCount);
	EntrySlot* slot = &H_Table.Entries[idx];
	U64 entry = slot->EntrySlots[0];
	int dbDepth = (entry >> 42) & 0x1F;
	uint dbKey2 = entry & 0x0FFFFFFF;

	//Always overwrite unless same hash with lower depth is stored.
	if (dbKey2 == key2) {
		if (depth >= dbDepth) {
			slot->EntrySlots[0] = pack;
			HashTableEntries++;
			return;
		}
		// not usefull info here when depth is less than before.
	}
	else if (entry == 0) { // empty, just store
		slot->EntrySlots[0] = pack;
		HashTableEntries++;
		return;
	}
	else { //key2 difference
		// move away the older ones one position
		memmove(&slot->EntrySlots[1], slot->EntrySlots, sizeof(U64) * (SLOTS - 1));
		slot->EntrySlots[0] = pack;
		HashTableEntries++;

		/*if (slot->EntrySlots[SLOTS - 1] != 0) {
			HashTableOverWrites++;
		}*/
	}
}


bool GetScoreFromHash(U64 hash, char depth, short* score, Move* pvMove, short best_black, short best_white) {
	uint idx = (uint)(hash % H_Table.EntryCount);
	uint key2 = hash & 0x0FFFFFFF;
	EntrySlot* slot = &H_Table.Entries[idx];
	for (int i = 0; i < SLOTS; i++)
	{
		U64 entry = slot->EntrySlots[i];
		uint dbKey = entry & 0x0FFFFFFF;
		int dbDepth = (entry >> 42) & 0x1F;
		if (dbKey == key2) {
			pvMove->From = (entry >> 49) & 0x3F;
			pvMove->To = (entry >> 55) & 0x3F;
			char promo = (entry >> 61) & 0x7;
			if (promo >= PromotionQueen && promo <= PromotionKnight)
				pvMove->MoveInfo = promo;
			else
				pvMove->MoveInfo = PlainMove;
			if (dbDepth >= depth)
			{
				*score = ((entry >> 28) & 0x3FFF) - MAX_SCORE;

				HashEntryType type = (entry >> 47) & 0x3;
				switch (type)
				{
				case BEST_BLACK:
					if (*score <= best_black) {
						*score = best_black;
						return true;
					}
					break;
				case BEST_WHITE:
					if (*score >= best_white) {
						*score = best_white;
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
	}
	return false;
}

bool GetBestMoveFromHash(U64 hash, Move* move) {
	//uint idx = (uint)((hash >> 32) % H_Table.EntryCount);
	uint idx = (uint)(hash % H_Table.EntryCount);
	uint key2 = hash & 0x0FFFFFFF;
	EntrySlot* slot = &H_Table.Entries[idx];
	for (int i = 0; i < SLOTS; i++)
	{
		U64 entry = slot->EntrySlots[i];
		uint dbKey = entry & 0x0FFFFFFF;
		if (dbKey == key2) {
			move->From = (entry >> 49) & 0x3F;
			move->To = (entry >> 55) & 0x3F;
			char promo = (entry >> 61) & 0x7;
			if (promo >= PromotionQueen && promo <= PromotionKnight)
				move->MoveInfo = promo;
			else
				move->MoveInfo = PlainMove;
			return true;
		}
	}

	return false;
}

void GenerateZobritsKeys() {
	StartHash = Llrand();
	for (int i = 0; i < 23; i++) //only using 16 of these, but King | BLACK is 22. See also PieceType enum.
		for (int s = 0; s < 64; s++)
			ZobritsPieceTypesSquares[i][s] = Llrand();
	//Setting nopiece to zeros. Will not affect hash.
	for (int i = 0; i < 64; i++)
		ZobritsPieceTypesSquares[0][i] = 0;

	ZobritsSides[0] = Llrand();
	ZobritsSides[1] = Llrand();
	for (int i = 0; i < 4; i++)
		ZobritsCastlingRights[i] = Llrand();

	ZobritsEnpassantFile[0] = 0; //no enpassant file
	for (int i = 1; i < 9; i++)
		ZobritsEnpassantFile[i] = Llrand();
}

void AllocateHashTable(uint megabytes) {
	free(H_Table.Entries);
	H_Table.EntryCount = (megabytes * 0x100000ULL) / sizeof(EntrySlot);
	H_Table.Entries = malloc(H_Table.EntryCount * sizeof(EntrySlot));
}

void ClearHashTable() {
	memset(H_Table.Entries, 0, H_Table.EntryCount * sizeof(EntrySlot));
	HashTableEntries = 0;
	HashTableOverWrites = 0;
}

void PrintHashStats() {
	//printf("Entries:    %d\n", HashTableEntries);
	//printf("Overwrites: %d\n", HashTableOverWrites);
	//printf("Capacity:   %d\n", H_Table.EntryCount * SLOTS);
	/*double d = (double)HashTableEntries / ((double)H_Table.EntryCount * SLOTS);
	printf("info hashfull %d\n", HashFull());
	fflush(stdout);*/
}

uint HashFull() {
	double d = (double)HashTableEntries / ((double)H_Table.EntryCount * SLOTS);
	return (uint)(d * 1000);
}
