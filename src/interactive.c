#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commons.h"
#include "platform.h"
#include "fen.h"
#include "utils.h"
#include "evaluation.h"
#include "search.h"
#include "moves.h"
#include "tests.h"
#include "interactive.h"

void ComputerMove()
{
	g_topSearchParams.MoveTime = 5000;
	MoveCoordinates move = Search(false);
	char sMove[6];
	CoordinatesToString(move, sMove);
	MakePlayerMove(sMove);
}

void ManualMove()
{
	printf("\nYour move (format e2e4 or e7e8q): ");
	char sMove[8];
	if (fgets(sMove, 8, stdin) == NULL)
		return;
	fseek(stdin, 0, SEEK_END);
	PlayerMove pm = MakePlayerMove(sMove);
	if (pm.Invalid)
		Stdout_wl("Invalid move");
}

void PrintOptions()
{
	printf("\nm: make move\n");
	printf("c: computer move\n");
	printf("t: run tests\n");
	printf("g: print game\n");
	printf("e: eval\n");
	printf("h: help\n");
	printf("q: quit\n");
}

int EnterInteractiveMode()
{
	char buffer[20];
	char in = ' ';
	PrintGame(&g_mainGame);
	PrintOptions();

	while (in != 'q')
	{
		// system("@cls||clear");
		printf("h for help> ");
		if (fgets(buffer, 20, stdin) == NULL)
			break;
		if (buffer[1] == '\n')
			in = buffer[0];
		fseek(stdin, 0, SEEK_END);
		switch (in)
		{
		case 'm':
			ManualMove();
			PrintGame(&g_mainGame);
			break;
		case 'c':
			ComputerMove();
			PrintGame(&g_mainGame);
			break;
		case 'g':
			PrintGame(&g_mainGame);
			break;
		case 't':
			runAllTests();
			break;
		case 'e':
			printf("Eval: %d\n", GetEval(&g_mainGame));
			break;
		case 'h':
			PrintOptions();
			break;
		case 'q':
			break;
		default:
			Stdout_wl("Not an option");
			break;
		}
	}

	return 0;
}

void PrintGame(Game *game)
{
	char top[] = "+---+---+---+---+---+---+---+---+";
	char rowLine[] = "+---+---+---+---+---+---+---+---+";
	char lastLine[] = "+---+---+---+---+---+---+---+---+";
	char vBorder = '|';

	printf("   %s\n", top);

	for (int r = 8 - 1; r >= 0; r--)
	{
		printf(" %d", r + 1);
		printf(" %c", vBorder);
		for (int f = 0; f < 8; f++)
		{
			printf(" ");
			PieceType piece = game->Squares[r * 8 + f];
			Side side = piece & 24;
			char c[] = {PieceChar(piece), 0};
			if (side == BLACK)
				ColorPrint(c, lightblue, black);
			else
				ColorPrint(c, yellow, black);

			if (f < 7)
				printf(" |");
			else
				printf(" %c\n", vBorder);
		}
		if (r > 0)
			printf("   %s\n", rowLine);
	}
	printf("   %s\n", lastLine);
	printf("     a   b   c   d   e   f   g   h  \n");
	printf("Hash: %llx\n", game->Hash);
	char fen[100];
	WriteFen(fen);
	printf("FEN: %s\n", fen);
}
