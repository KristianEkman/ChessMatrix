#pragma once
#include <stdlib.h>
#include <Windows.h>
#include "commons.h"

bool g_Stopped;
TopSearchParams g_topSearchParams;
void SetSearchDefaults();
MoveCoordinates Search(bool async);
void InitLmr();
