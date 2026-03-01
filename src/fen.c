#include "fen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "position.h"
#include "utils.h"

char PieceChar(PieceType pieceType)
{
	Side color = pieceType & (BLACK | WHITE);
	PieceType pt = pieceType & 7;
	switch (pt)
	{
	case PAWN:
		return color == WHITE ? 'P' : 'p';
	case ROOK:
		return color == WHITE ? 'R' : 'r';
	case KNIGHT:
		return color == WHITE ? 'N' : 'n';
	case BISHOP:
		return color == WHITE ? 'B' : 'b';
	case QUEEN:
		return color == WHITE ? 'Q' : 'q';
	case KING:
		return color == WHITE ? 'K' : 'k';
	default:
		return ' ';
	}
}

PieceType parsePieceType(char c)
{
	switch (c)
	{
	case 'p':
		return PAWN | BLACK;
	case 'r':
		return ROOK | BLACK;
	case 'b':
		return BISHOP | BLACK;
	case 'n':
		return KNIGHT | BLACK;
	case 'q':
		return QUEEN | BLACK;
	case 'k':
		return KING | BLACK;
	case 'P':
		return PAWN | WHITE;
	case 'R':
		return ROOK | WHITE;
	case 'B':
		return BISHOP | WHITE;
	case 'N':
		return KNIGHT | WHITE;
	case 'Q':
		return QUEEN | WHITE;
	case 'K':
		return KING | WHITE;
	default:
		return NOPIECE;
	}
}

Side parseSide(char c)
{
	switch (c)
	{
	case 'w':
		return WHITE;
	case 'b':
		return BLACK;
	default:
		return WHITE;
	}
}

void ReadFen(const char *fen)
{
	InitPieceList();

	// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
	for (int i = 0; i < 64; i++)
		g_mainGame.Squares[i] = NOPIECE;
	int index = 0;
	int file = 0;
	int rank = 7;
	while (fen[index] != ' ' && fen[index])
	{
		char c = fen[index];
		index++;
		if (isdigit(c))
		{
			int dig = ParseChar(c);
			file += dig;
		}
		else if (c == '/')
		{
			rank--;
			file = 0;
		}
		else
		{
			PieceType pt = parsePieceType(c);
			InitPiece(file, rank, pt & 7, pt & 24);
			file++;
		}
	}

	index++;
	g_mainGame.Side = parseSide(fen[index]);
	g_mainGame.Side01 = g_mainGame.Side >> 4;
	index++;
	index++;
	g_mainGame.State = 0;
	while (fen[index] != ' ')
	{
		switch (fen[index])
		{
		case 'K':
			g_mainGame.State |= WhiteCanCastleShort;
			break;
		case 'Q':
			g_mainGame.State |= WhiteCanCastleLong;
			break;
		case 'k':
			g_mainGame.State |= BlackCanCastleShort;
			break;
		case 'q':
			g_mainGame.State |= BlackCanCastleLong;
			break;
		default:
			break;
		}
		index++;
	}
	index++;
	char enpFile = fen[index] - 'a';
	if (enpFile >= 0 && enpFile <= 8)
		g_mainGame.State |= (enpFile + 1);

	index++; // skipping enp rank, not important

	// tokens
	int len = strlen(fen);
	char fenCopy[100];
	memcpy(fenCopy, fen, len + 1);
	int tokNo = 0;
	char *token = NULL;
#ifdef _WIN32
	char *context;
	token = strtok_s(fenCopy, " ", &context);
#else
	char *context = NULL;
	token = strtok_r(fenCopy, " ", &context);
#endif
	while (token != NULL)
	{
		tokNo++;
		if (tokNo == 5)
			g_mainGame.FiftyMoveRuleCount = atoi(token);
#ifdef _WIN32
		token = strtok_s(NULL, " ", &context);
#else
		token = strtok_r(NULL, " ", &context);
#endif
	}

	for (int i = 0; i < 64; i++)
	{
		if ((g_mainGame.Squares[i] & 7) == KING)
		{
			int color = (g_mainGame.Squares[i] >> 4);
			g_mainGame.KingSquares[color] = i;
		}
	}
	FixPieceChain(&g_mainGame);
	InitScores();
	InitHash();
}

void WriteFen(char *fenBuffer)
{
	int index = 0;
	for (int rank = 8 - 1; rank >= 0; rank--)
	{
		for (int file = 0; file < 8; file++)
		{
			int emptyCount = 0;
			while (g_mainGame.Squares[rank * 8 + file] == NOPIECE && file < 8)
			{
				emptyCount++;
				file++;
			}

			if (emptyCount > 0)
			{
				fenBuffer[index++] = '0' + emptyCount;
				file--;
			}
			else
			{
				fenBuffer[index++] = PieceChar(g_mainGame.Squares[rank * 8 + file]);
			}
		}
		if (rank > 0)
			fenBuffer[index++] = '/';
	}
	fenBuffer[index++] = ' ';
	fenBuffer[index++] = g_mainGame.Side == WHITE ? 'w' : 'b';
	fenBuffer[index++] = ' ';
	if (g_mainGame.State & WhiteCanCastleShort)
		fenBuffer[index++] = 'K';
	if (g_mainGame.State & WhiteCanCastleLong)
		fenBuffer[index++] = 'Q';
	if (g_mainGame.State & BlackCanCastleShort)
		fenBuffer[index++] = 'k';
	if (g_mainGame.State & BlackCanCastleLong)
		fenBuffer[index++] = 'q';
	fenBuffer[index++] = ' ';

	char noFile = 'a' - 1;
	char enPassantFile = (g_mainGame.State & 15) + noFile;
	if (enPassantFile == noFile)
		fenBuffer[index++] = '-';
	else
	{
		fenBuffer[index++] = enPassantFile;
		fenBuffer[index++] = g_mainGame.Side == WHITE ? '6' : '3';
	}
	fenBuffer[index++] = ' ';
	index += snprintf(&fenBuffer[index], 16, "%d", g_mainGame.FiftyMoveRuleCount);
	fenBuffer[index] = '\0';
}
