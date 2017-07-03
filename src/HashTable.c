#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "basic_structs.h"

//Very Simple hash table
//Array of 268435455, 2^28 or FFFFFFF entries usining 28 first bits of hash as hopefully uniqueue index.
//One entry is 4byte so hashtable is fixed at exactly 1 gb.
HashTableEntry index[0xFFFFFFF];

void addEntry(unsigned long long hash, short score) {
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

//
//int main() {
//	clock_t start = clock();
//
//	for (int i = 0; i < 5; i++)
//	{
//		printf("%d\n", RAND_MAX);
//		unsigned long long r = llrand();
//
//		printf("%llx\n", r);
//		int entry = r >> 31;
//		addEntry(r, entry);
//	}
//	clock_t stop = clock();
//	float secs = (float)(stop - start) / CLOCKS_PER_SEC;
//	printf("%.9fs\n", secs);
//	printf("Done");
//}
