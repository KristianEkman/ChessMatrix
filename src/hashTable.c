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

HashTableEntry HashTable[IndexLength + 1][SlotCount];
void addHashScore(unsigned long long hash, short score, char depth) {
	//return;
	unsigned int idx = (unsigned int)(hash & IndexLength);
	HashTableEntry * entry = HashTable[idx];
	
	for (int i = 0; i < SlotCount; i++)
	{
		if (entry[i].Key2 == 0) { //empty entry, add it
			entry[i].Score = score;
			entry[i].Key2 = (unsigned int)(hash >> 28);
			entry->Depth = depth;
			//HashTableEntries++;
			return;
		}
		unsigned int key2 = (unsigned int)(hash >> 28);
		if (entry[i].Key2 == key2 && depth > entry[i].Depth) {
			
			entry[i].Score = score;
			entry[i].Key2 = key2;
			entry->Depth = depth;
			return;
		}
	}
	//HashTableFullCount++;
}

short getScoreFromHash(unsigned long long hash, bool * empty, int * depth) {
	/**empty = true;
	return 0;*/
	unsigned int idx = (unsigned int)(hash & IndexLength);
	HashTableEntry * entry = HashTable[idx];
	unsigned int key2 = (unsigned int)(hash >> 28);
	for (int i = 0; i < SlotCount; i++)
	{
		if (entry[i].Key2 == key2)
		{
			//HashTableMatches++;
			*depth = entry[i].Depth;
			*empty = false;
			return entry[i].Score;
		}
	}
	*empty = true;
	return 0;
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
	HashTableEntry emptyEntry;
	emptyEntry.Depth = 0;
	emptyEntry.Key2 = 0;
	emptyEntry.Score = 0;
	for (size_t i = 0; i < IndexLength; i++)
		for (size_t s = 0; s < SlotCount; s++)
		{
			HashTable[i][s] = emptyEntry;
		}
	/*HashTableFullCount = 0;
	HashTableEntries = 0;
	HashTableMatches = 0;*/
}
