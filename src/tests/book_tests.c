#include <stdio.h>
#include <string.h>

#include "helpers.h"

#include "../main.h"
#include "../fen.h"
#include "../commons.h"
#include "../position.h"
#include "../book.h"

#define NO_BOOK_MOVE 255

TEST(BookLoadTest)
{
	// Ensure clean state
	CloseBook();
	OwnBook = true;
	StartPosition();

	LoadBook("src/openings.abk");

	MoveCoordinates move = BestBookMove(&g_mainGame);
	Assert(move.From != NO_BOOK_MOVE, "BookLoadTest: expected a book move from start position");
	Assert(move.To   != NO_BOOK_MOVE, "BookLoadTest: expected a valid To square from start position");

	CloseBook();
}

TEST(BookDisabledTest)
{
	// With OwnBook=false, BestBookMove must return NO_BOOK_MOVE regardless of loaded data
	CloseBook();
	OwnBook = false;
	StartPosition();

	// Load the data but keep OwnBook off
	OwnBook = true;
	LoadBook("src/openings.abk");
	OwnBook = false;

	MoveCoordinates move = BestBookMove(&g_mainGame);
	Assert(move.From == NO_BOOK_MOVE, "BookDisabledTest: expected no book move when OwnBook is false");

	CloseBook();
}

TEST(BookDoubleLoadTest)
{
	// Loading the book twice must not crash or duplicate data.
	// The guard in LoadBook (hashBook != NULL) should prevent re-loading.
	CloseBook();
	OwnBook = true;
	StartPosition();

	LoadBook("src/openings.abk");
	LoadBook("src/openings.abk"); // second call must be a no-op

	MoveCoordinates move = BestBookMove(&g_mainGame);
	Assert(move.From != NO_BOOK_MOVE, "BookDoubleLoadTest: expected a valid book move after double load");

	CloseBook();
}

TEST(BookCloseAndReloadTest)
{
	// After CloseBook, loading again should still work.
	CloseBook();
	OwnBook = true;
	StartPosition();

	LoadBook("src/openings.abk");
	CloseBook();
	LoadBook("src/openings.abk");

	MoveCoordinates move = BestBookMove(&g_mainGame);
	Assert(move.From != NO_BOOK_MOVE, "BookCloseAndReloadTest: expected a valid book move after close+reload");

	CloseBook();
	OwnBook = false;
}
