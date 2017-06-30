#include <stdio.h>
#include <time.h>
#include <stdlib.h>

//Array of 16 777 215 pointers to stort of smaller list of entries (67MB)
//4Byte entry och 8 entries för varje pointer på varje ger max storlek 537 MB 
//och ca 134 miljoner entries. ca 26 s sökning.
int *index[0xFFFFFF];

addEntry(long long hash, short score) {
	int entry = score;
	int i = (int)(hash & 0xFFFFFF);
	int  * p = index[i];
	if (p == NULL) {
		index[i] = malloc(8 * sizeof(int));
		*index[i] = score;
	}
	//else find next empty and write it
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
