//#include <stdio.h>
//#include "sort.h"
//#include "commons.h"
//
//int partition(Move[], int, int);
//
//int partitionDescending(Move[], int, int);
//
//void QuickSort(Move moves[], int left, int right)
//{
//	int j;
//	if (left < right)
//	{
//		j = partition(moves, left, right);
//		QuickSort(moves, left, j - 1);
//		QuickSort(moves, j + 1, right);
//	}
//}
//
//int partition(Move moves[], int left, int right) {
//	Move pivot, t;
//	int i, j;
//	pivot = moves[left];
//	i = left;
//	j = right + 1;
//
//	while (1)
//	{
//		do {
//			++i;
//		} while (moves[i].Score <= pivot.Score && i <= right);
//
//		do {
//			--j;
//		}
//		while (moves[j].Score > pivot.Score);
//		
//		if (i >= j)
//			break;
//
//		t = moves[i];
//		moves[i] = moves[j];
//		moves[j] = t;
//	}
//	t = moves[left];
//	moves[left] = moves[j];
//	moves[j] = t;
//	return j;
//}
//
//void QuickSortDescending(Move moves[], int left, int right)
//{
//	int j;
//	if (left < right)
//	{
//		j = partitionDescending(moves, left, right);
//		QuickSortDescending(moves, left, j - 1);
//		QuickSortDescending(moves, j + 1, right);
//	}
//}
//
//int partitionDescending(Move moves[], int left, int right) {
//	Move pivot, t;
//	int i, j;
//	pivot = moves[left];
//	i = left; j = right + 1;
//
//	while (1)
//	{
//		do {
//			++i;
//		} while (-moves[i].Score <= -pivot.Score && i <= right);
//
//		do {
//			--j;
//		} while (-moves[j].Score > -pivot.Score);
//
//		if (i >= j)
//			break;
//
//		t = moves[i];
//		moves[i] = moves[j];
//		moves[j] = t;
//	}
//	t = moves[left];
//	moves[left] = moves[j];
//	moves[j] = t;
//	return j;
//}