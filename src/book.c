#include<stdio.h>
#include<stdlib.h>
#include "book.h"
#include "commons.h"
#include "utils.h"
#include <sys/stat.h>
#include <sys/types.h>

#define NO_BOOK_MOVE 255

bool OwnBook = false;

// see https://www.chessprogramming.org/ABK

HashBookEntry* hashBook = NULL;
int entryCount = 0;
char crap[25200] = { 0 };
void PlayBookPosition(Game* game, int bookIndex);

void LoadBook(char* fileName) {
	struct stat st;
	unsigned long fileSize;
	if (stat(fileName, &st) == 0)
		fileSize = st.st_size;
	else
		return;

	int entrySize = sizeof(BookEntry);
	int crapSize = 900 * entrySize;
	entryCount = (fileSize - crapSize) / entrySize;

	FILE* file = fopen(fileName, "rb");
	fread(crap, crapSize, 1, file);
	BookEntry* bookFile = malloc(sizeof(BookEntry) * entryCount);
	fread(bookFile, entrySize, entryCount, file);
	fclose(file);

	hashBook = malloc(sizeof(HashBookEntry) * entryCount);
	for (int i = 0; i < entryCount; i++)
	{
		hashBook[i].bookEntry = bookFile[i];

		if (hashBook[i].bookEntry.nextMove < 900 && hashBook[i].bookEntry.nextMove != -1)
			printf("Unexptected nextMove (%d) index. Below 900.\n", hashBook[i].bookEntry.nextMove);
		hashBook[i].bookEntry.nextMove -= 900;

		if (hashBook[i].bookEntry.nextSibling < 900 && hashBook[i].bookEntry.nextSibling != -1)
			printf("Unexptected nextSibling (%d) index. Below 900.\n", hashBook[i].bookEntry.nextSibling);
		hashBook[i].bookEntry.nextSibling -= 900;
		hashBook[i].positionHash = 0;
	}
	free(bookFile);

	PlayBookPosition(&g_mainGame, 0);
	int unresolved = 0;
	for (int i = 0; i < entryCount; i++)
		if (hashBook[i].positionHash == 0)
			unresolved++;
	if (unresolved > 1)
		printf("%d unresolved hash book entries.\n", unresolved);
	printf("book loaded\n");
}

void PlayBookPosition(Game* game, int bookIndex) {
	// plays all moves in book and stores its hash key for lookup later.
	hashBook[bookIndex].positionHash = game->Hash;

	HashBookEntry hbe = hashBook[bookIndex];
	Move move;
	move.From = hbe.bookEntry.from;
	move.To = hbe.bookEntry.to;
	char sMove[6];
	MoveToString(move, sMove);
	PlayerMove pMove = MakePlayerMoveOnThread(game, sMove);
	if (pMove.Invalid)
	{
		printf("Invalid move in book\n");
		return;
	}

	if (hbe.bookEntry.nextMove > -1)
		PlayBookPosition(game, hbe.bookEntry.nextMove);
	UnMakePlayerMoveOnThread(game, pMove);

	if (hbe.bookEntry.nextSibling > -1) {
		PlayBookPosition(game, hbe.bookEntry.nextSibling);
	}
}

void CloseBook() {
	free(hashBook);
}

MoveCoordinates BestBookMove(Game* game)
{
	MoveCoordinates foundMove;
	foundMove.From = NO_BOOK_MOVE;
	foundMove.To = NO_BOOK_MOVE;
	if (!OwnBook)
		return foundMove;
	double bestRatio = -100000;
	int maxSum = 0;
	int moveCount = 0;
	for (int i = 0; i < entryCount; i++)
	{
		if (game->Hash == hashBook[i].positionHash) {
			
			moveCount++;
			BookEntry bookEntry = hashBook[i].bookEntry;

			/*Move tmov;
			tmov.From = bookEntry.from;
			tmov.To = bookEntry.to;
			char smove[5];
			MoveToString(tmov, smove);
			printf("%s\n", smove);*/

			int sum = bookEntry.nwon + bookEntry.nlost;
			if (sum > maxSum)
				maxSum = sum;
			//Skips moves that are unusual.
			if ((double)sum / (double)maxSum < 0.01)
				continue;
			int won = bookEntry.nwon - bookEntry.nlost;
			double ratio = (double)won / (double)sum;
			if (ratio <= bestRatio || ratio < 0)
				continue;
			bestRatio = ratio;
			foundMove.From = bookEntry.from;
			foundMove.To = bookEntry.to;
		}
	}

	if (moveCount == 0)
		return foundMove;

	printf("Selecting best book move from %d moves\n", moveCount);

	//This is tricky. It resolves some more information needed. I.e. index of piece being moved.
	char sMove[6];
	CoordinatesToString(foundMove, sMove);
	PlayerMove pm = MakePlayerMoveOnThread(game, sMove);
	UnMakePlayerMoveOnThread(game, pm);
	if (pm.Invalid) {
		printf("Invalid move found in book\n");
		MoveCoordinates noMove;
		noMove.From = NO_BOOK_MOVE;
		noMove.To = NO_BOOK_MOVE;
		return noMove;
	}
	return foundMove;
}

// Selects a random good book move
MoveCoordinates RandomBookMove(Game* game)
{
	MoveCoordinates foundMoves[5];
	foundMoves[0].From = NO_BOOK_MOVE;
	foundMoves[0].To = NO_BOOK_MOVE;
	if (!OwnBook)
		return foundMoves[0];
	int moveCount = 0;
	for (int i = 0; i < entryCount; i++)
	{
		if (game->Hash == hashBook[i].positionHash) {
			BookEntry bookEntry = hashBook[i].bookEntry;
			if (bookEntry.nwon > bookEntry.nlost && moveCount < 5)
			{
				foundMoves[moveCount].From = bookEntry.from;
				foundMoves[moveCount].To = bookEntry.to;
				moveCount++;
			}
		}
	}

	if (moveCount == 0)
		return foundMoves[0];
	int random = RandomInt(0, moveCount - 1);
	printf("Selecting best book move from %d moves\n", moveCount);

	//This is tricky. It resolves some more information needed. I.e. index of piece being moved.
	char sMove[6];
	CoordinatesToString(foundMoves[random], sMove);
	PlayerMove pm = MakePlayerMoveOnThread(game, sMove);
	UnMakePlayerMoveOnThread(game, pm);
	if (pm.Invalid) {
		printf("Invalid move found in book\n");
		MoveCoordinates noMove;
		noMove.From = NO_BOOK_MOVE;
		noMove.To = NO_BOOK_MOVE;
		return noMove;
	}
	return foundMoves[random];
}