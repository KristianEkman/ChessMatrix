#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "basic_structs.h"
#include "utils.h"
#include "HashTable.h"

//Very Simple hash table
//Array of 268435455, 2^28 or FFFFFFF entries usining 28 first bits of hash as hopefully uniqueue index.
//One entry is 4byte so hashtable is fixed at exactly 1 gb.
HashTableEntry index[0xFFFFFFF];

void addEntry(unsigned long long hash, short score, char depth) {
	int i = (int)(hash & 0xFFFFFFF);
	index[i] = score;
	return;
	//todo
	//else check if score is from a deeper iteration, then replace it
}

short getScoreFromHash(unsigned long long hash) {
	int i = (int)(hash & 0xFFFFFFF);
	return index[i];
}

void GenerateZobritsKeys() {
	for (int i = 0; i < 16; i++)
	{
		for (int s = 0; s < 64; s++)
		{
			ZobritsPieceTypesSquares[i][s] = llrand();
		}
	}
	ZobritsSides[0] = llrand();
	ZobritsSides[1] = llrand();
	for (int i = 0; i < 4; i++)
	{
		ZobritsCastlingRights[i] = llrand();
	}
}
