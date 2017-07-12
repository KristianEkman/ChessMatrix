#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "basic_structs.h"
#include "utils.h"
#include "hashTable.h"

//Very Simple hash table
//Array of 16777215, 2^24 or FFFFFF entries using 24 first bits of hash as an index.
//At each index there are three slots. Max size of HashTable is 384 MB.
//Can be incerased by 128MB by increasing slot count by one

HashTableEntry HashTable[IndexLength][SlotCount];

void addEntry(unsigned long long hash, short score, char depth) {
	int idx = (int)(hash & IndexLength);
	HashTableEntry * entry = HashTable[idx];
	
	for (int i = 0; i < SlotCount; i++)
	{
		if (entry[i].Key2 == 0) { //empty entry, add it
			entry[i].Score = score;
			entry[i].Key2 = (int)(hash >> 32);
			entry->Depth = depth;
			HashTable[idx][i] = entry[i];
			return;
		}
		int key2 = (int)(hash >> 32);
		if (entry[i].Key2 == key2 && depth > entry[i].Depth) {
			entry[i].Score = score;
			entry[i].Key2 = (int)(hash >> 32);
			entry->Depth = depth;
			HashTable[idx][i] = entry[i];
			return;
		}
	}
	
	//todo, mabe try with pointer to entry for performance
	//if there is a Key2 missmatch, count and exit
	//
	
	//todo
	//else check if score is from a deeper iteration, then replace it
}

short getScoreFromHash(unsigned long long hash) {
	int idx = (int)(hash & 0xFFFFFF);
	HashTableEntry * entry = HashTable[idx];
	int key2 = (int)(hash >> 32);
	for (int i = 0; i < SlotCount; i++)
	{
		if (entry[i].Key2 == key2)
		{
			return HashTable[idx][i].Score;
		}
	}
	return 40000;
}

void GenerateZobritsKeys() {
	for (int i = 0; i < 23; i++)
		for (int s = 0; s < 64; s++)
			ZobritsPieceTypesSquares[i][s] = llrand();
	ZobritsSides[0] = llrand();
	ZobritsSides[1] = llrand();
	for (int i = 0; i < 4; i++)
		ZobritsCastlingRights[i] = llrand();

	ZobritsEnpassantFile[0] = 0; //no enpassant file
	for (int i = 1; i < 8; i++)
		ZobritsEnpassantFile[i] = llrand();
}

void ClearHashTable() {
	HashTableEntry emptyEntry;
	emptyEntry.Depth = 0;
	emptyEntry.Key2 = 0;
	emptyEntry.Score = 0;
	for (size_t i = 0; i < IndexLength; i++)
		for (size_t s = 0; s < SlotCount; s++)
			HashTable[i][s] = emptyEntry;
}
