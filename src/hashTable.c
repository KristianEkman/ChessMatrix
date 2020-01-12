#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <Windows.h>
#include "basic_structs.h"
#include "utils.h"
#include "hashTable.h"

//Very Simple hash table
//Array of 16777215, 2^24 or FFFFFF entries using 24 first bits of hash as an index.
//At each index there are three slots. Max size of HashTable is 384 MB.
//Can be incerased by 128MB by increasing slot count by one.

//		bits shift >>   mask
//key2	31	 0	        7FFF FFFF
//score	14	 31	        3FFF
//depth	5	 45	        1F
//type	2	 50	        3
//from	6	 52	        3F
//to	6	 58	        3F

HashEntry HashTable[IndexLength + 1][SlotCount];
void addHashScore(U64 hash, short score, char depth, HashEntryType type, char from, char to) {
	//return;
	unsigned int idx = (unsigned int)(hash & IndexLength);
	HashEntry* entry = HashTable[idx];

	int key2 = hash & 0xFFFFFFFF;

	for (int i = 0; i < SlotCount; i++)
	{
		if (entry[i].Key2 == 0) { //empty entry, add it			
			entry[i].Score = score;
			entry[i].Key2 = key2;
			entry[i].Depth = depth;
			entry[i].From = from;
			entry[i].To = to;
			entry[i].Type = type;
			//HashTableEntries++;
			return;
		}
		if (entry[i].Key2 == key2 && depth >= entry[i].Depth) {
			entry[i].Score = entry;
			entry[i].Key2 = key2;
			entry[i].Depth = depth;
			entry[i].From = from;
			entry[i].To = to;
			entry[i].Type = type;
			return;
		}
	}
	//HashTableFullCount++;
}

bool getScoreFromHash(U64 hash, char depth, short* score, char* from, char* to, short alpha, short beta) {
	unsigned int idx = (unsigned int)(hash & IndexLength);
	HashEntry* entries = HashTable[idx];
	int key2 = hash & 0xFFFFFFFF;
	for (int i = 0; i < SlotCount; i++)
	{
		HashEntry* entry = &entries[i];
		if (entry->Key2 == key2)
		{
			*from = entry->From;
			*to = entry->To;
			if (entry[i].Depth >= depth) {
				*score = entry[i].Score;

				// adjust mate scores with in_deep

				switch (entry->Type)
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
			break;
			//HashTableMatches++;
		}
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
void ClearHashTable() {
	HashEntry emptyEntry;
	emptyEntry.Depth = 0;
	emptyEntry.From = 0;
	emptyEntry.Key2 = 0;
	emptyEntry.Score = 0;
	emptyEntry.To = 0;
	emptyEntry.Type = 0;
	for (size_t i = 0; i < IndexLength; i++)
		for (size_t s = 0; s < SlotCount; s++)
		{
			HashTable[i][s] = emptyEntry;
		}
	/*HashTableFullCount = 0;
	HashTableEntries = 0;
	HashTableMatches = 0;*/
}
