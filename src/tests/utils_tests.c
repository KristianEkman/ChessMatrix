#include <stdio.h>
#include <time.h>
#include <string.h>

#include "helpers.h"

#include "../main.h"
#include "../fen.h"
#include "../commons.h"
#include "../utils.h"
#include "../hashTable.h"
#include "../evaluation.h"
#include "../search.h"
#include "../moves.h"
#include "../platform.h"
#include "../bitboards.h"

void indexOfTest()
{
	char *s2 = "Kristian Ekman";
	Assert(IndexOf(s2, "Ekman") == 9, "index of failed");
}

void containsNotTest()
{
	char *s2 = "Kristian Ekman";
	AssertNot(Contains(s2, "annika"), "containsNotTest failed");
}