#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commons.h"
#include "main.h"
#include "utils.h"
#include "hashTable.h"
#include "timeControl.h"
#include "search.h"
#include "position.h"
#include "fen.h"
#include "moves.h"
#include "book.h"
#include "interactive.h"
#include "errorHandling.h"
#include "uci.h"

extern char Version[];

void EnterUciMode()
{
#define UCI_BUF_SIZE 5000
	char buf[UCI_BUF_SIZE];
	if (fgets(buf, UCI_BUF_SIZE, stdin) == NULL)
	{
		EmitUciError("stdin closed before first command");
		return;
	}
	while (!Streq(buf, "quit\n"))
	{
		if (Streq(buf, "ucinewgame\n"))
		{
			StopSearch();
			ClearHashTable();
			ResetDepthTimes();
			Stdout_wl("new game");
		}
		else if (Streq(buf, "uci\n"))
		{
			printf("id name ChessMatrix %s\n", Version);
			Stdout_wl("id author Kristian Ekman");
			Stdout_wl("option name Hash type spin default 1024 min 1 max 2048");
			Stdout_wl("option name OwnBook type check default false");
			Stdout_wl("uciok");
		}
		else if (Streq(buf, "isready\n"))
		{
			Stdout_wl("readyok");
		}
		else if (StartsWith(buf, "setoption name Hash value"))
		{
			// setoption name Hash value 32
			char *token = strtok(buf, " ");
			while (token != NULL)
			{
				if (Streq(token, "value"))
				{
					token = strtok(NULL, " ");
					int mb = atoi(token);
					if (mb >= 1 && mb <= 2048)
					{ // actual limit is 32760mb
						AllocateHashTable(mb);
						Stdout_wl("new hash size set");
					}
					else
					{
						Stdout_wl("invalid Hash size value (1 - 2048)");
					}
					break;
				}
				token = strtok(NULL, " ");
			}
		}
		else if (StartsWith(buf, "setoption name OwnBook value"))
		{
			// setoption name OwnBook value false
			char *token = strtok(buf, " ");
			while (token != NULL)
			{
				if (Streq(token, "value"))
				{
					token = strtok(NULL, " ");
					if (Streq("true\n", token))
					{
						OwnBook = true;
						Stdout_wl("Opening book switched on");
					}
					else if (Streq("false\n", token))
					{
						OwnBook = false;
						Stdout_wl("Opening book switched off");
					}
					break;
				}
				token = strtok(NULL, " ");
			}
		}
		else if (StartsWith(buf, "position "))
		{
			StopSearch();
			// postion fen | moves
			int movesPos = IndexOf(buf, " moves");
			if (Contains(buf, " fen "))
			{
				char *pFen = strstr(buf, " fen ");
				int fenPos = pFen - buf + 5;

				if (movesPos == -1)
				{ // note sure if "moves" string is mandatory
					movesPos = (int)strlen(buf) - 1;
				}

				int fenLength = movesPos - fenPos; // position fen xxx...yyy moves
				char fen[256];
				memcpy(fen, pFen + 5, fenLength);
				fen[fenLength] = '\0';
				ReadFen(fen);
			}

			if (Contains(buf, "startpos"))
			{
				StartPosition();
			}

			if (Contains(buf, " moves "))
			{
				char *pMoves = strstr(buf, " moves ");
				char *token = strtok(pMoves, " ");
				while (token != NULL)
				{
					if (!Streq(token, "moves") && strlen(token) >= 4)
						MakePlayerMove(token);
					token = strtok(NULL, " ");
				}
				Stdout_wl("string position parsed");
				AssertGame(&g_mainGame);
			}
		}
		else if (StartsWith(buf, "go "))
		{
			SetSearchDefaults();
			if (Contains(buf, "infinite"))
			{
				g_topSearchParams.MaxDepth = MAX_DEPTH;
				g_topSearchParams.TimeControl = false;
				Search(true);
			}
			else
			{
				char *token = strtok(buf, " ");
				while (token != NULL)
				{
					if (Streq(token, "depth"))
					{
						char *depth = strtok(NULL, " ");
						uint dep;
						if (sscanf(depth, "%d", &dep) == 1)
							g_topSearchParams.MaxDepth = dep;
					}
					else if (Streq(token, "movetime"))
					{
						char *movetime = strtok(NULL, " ");
						int time = 0;
						if (sscanf(movetime, "%d", &time) == 1)
							g_topSearchParams.MoveTime = time;
					}
					else if (Streq(token, "wtime"))
					{
						g_topSearchParams.TimeControl = true;
						char *wtime = strtok(NULL, " ");
						sscanf(wtime, "%d", &g_topSearchParams.WhiteTimeLeft);
						// go wtime 900000 btime 900000 winc 0 binc 0
					}
					else if (Streq(token, "btime"))
					{
						g_topSearchParams.TimeControl = true;
						char *btime = strtok(NULL, " ");
						sscanf(btime, "%d", &g_topSearchParams.BlackTimeLeft);
					}
					else if (Streq(token, "winc"))
					{
						char *winc = strtok(NULL, " ");
						sscanf(winc, "%d", &g_topSearchParams.WhiteIncrement);
					}
					else if (Streq(token, "binc"))
					{
						char *binc = strtok(NULL, " ");
						sscanf(binc, "%d", &g_topSearchParams.BlackIncrement);
					}
					else if (Streq(token, "movestogo"))
					{
						char *binc = strtok(NULL, " ");
						sscanf(binc, "%d", &g_topSearchParams.MovesTogo);
					}
					token = strtok(NULL, " ");
				}

				SetMoveTimeFallBack(g_mainGame.Side);

				fflush(stdout);
				Search(true);
			}
		}
		else if (Streq(buf, "stop\n"))
		{
			StopSearch();
		}
		else if (Streq(buf, "i\n"))
		{
			EnterInteractiveMode();
			Stdout_wl("In uci mode");
		}
		else if (Streq(buf, "\n"))
		{
		}
		else
		{
			Stdout_wl("unknown command");
		}
		if (fgets(buf, UCI_BUF_SIZE, stdin) == NULL)
		{
			EmitUciError("stdin closed while waiting for command");
			break;
		}
	}
	if (Streq(buf, "quit\n"))
		MarkQuitRequested();
}
