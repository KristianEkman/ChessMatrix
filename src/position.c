#include "position.h"

#include <stdio.h>
#include <stdlib.h>
#include "bitboards.h"
#include "patterns.h"
#include "hashTable.h"

void FixPieceChain(Game *game)
{
	for (int s = 0; s < 2; s++)
	{
		Piece *lastOn = NULL;
		for (int p = 15; p >= 0; p--)
		{
			Piece *piece = &game->Pieces[s][p];
			// It is not possible to include a piece in the piece chain if it is Off at this stage.
			// But this is before the search starts

			// The real linking
			if (!piece->Off)
			{
				piece->Next = lastOn;
				if (lastOn)
					lastOn->Prev = piece;
				lastOn = piece;
			}
		}

		if (game->Pieces[s][0].Off)
		{
			printf("King piece is not on the table. This is not allowed\n");
			fflush(stdout);
		}
	}
}

void StartPosition()
{
	InitPieceList();

	for (int i = 0; i < 64; i++)
		g_mainGame.Squares[i] = NOPIECE;

	InitPiece(0, 0, ROOK, WHITE);
	InitPiece(1, 0, KNIGHT, WHITE);
	InitPiece(2, 0, BISHOP, WHITE);
	InitPiece(3, 0, QUEEN, WHITE);
	InitPiece(4, 0, KING, WHITE);
	g_mainGame.KingSquares[0] = 4;
	InitPiece(5, 0, BISHOP, WHITE);
	InitPiece(6, 0, KNIGHT, WHITE);
	InitPiece(7, 0, ROOK, WHITE);

	for (int i = 0; i < 8; i++)
		InitPiece(i, 1, PAWN, WHITE);

	InitPiece(0, 7, ROOK, BLACK);
	InitPiece(1, 7, KNIGHT, BLACK);
	InitPiece(2, 7, BISHOP, BLACK);
	InitPiece(3, 7, QUEEN, BLACK);
	InitPiece(4, 7, KING, BLACK);
	g_mainGame.KingSquares[1] = 60;
	InitPiece(5, 7, BISHOP, BLACK);
	InitPiece(6, 7, KNIGHT, BLACK);
	InitPiece(7, 7, ROOK, BLACK);

	for (int i = 0; i < 8; i++)
		InitPiece(i, 6, PAWN, BLACK);
	g_mainGame.Side = WHITE;
	g_mainGame.Side01 = 0;

	g_mainGame.State = WhiteCanCastleLong | WhiteCanCastleShort | BlackCanCastleLong | BlackCanCastleShort;
	g_mainGame.Material[0] = 0;
	g_mainGame.Material[1] = 0;
	g_mainGame.PositionHistoryLength = 0;

	InitHash();
	InitScores();
	g_mainGame.FiftyMoveRuleCount = 0;
	SyncGameBitboards(&g_mainGame);
}

void InitPieceList()
{
	char side[2] = {WHITE, BLACK};
	for (int s = 0; s < 2; s++)
	{
		g_mainGame.Pieces[s][0].Type = KING | side[s];

		for (int p = 1; p < 9; p++)
			g_mainGame.Pieces[s][p].Type = PAWN | side[s];

		g_mainGame.Pieces[s][9].Type = ROOK | side[s];
		g_mainGame.Pieces[s][10].Type = ROOK | side[s];

		g_mainGame.Pieces[s][11].Type = KNIGHT | side[s];
		g_mainGame.Pieces[s][12].Type = KNIGHT | side[s];

		g_mainGame.Pieces[s][13].Type = BISHOP | side[s];
		g_mainGame.Pieces[s][14].Type = BISHOP | side[s];

		g_mainGame.Pieces[s][15].Type = QUEEN | side[s];

		for (int p = 0; p < 16; p++)
		{
			g_mainGame.Pieces[s][p].Off = true;
			g_mainGame.Pieces[s][p].MoveCount = 0;
			g_mainGame.Pieces[s][p].Index = p;
			if (p < 15)
				g_mainGame.Pieces[s][p].Next = &g_mainGame.Pieces[s][p + 1];
			else
				g_mainGame.Pieces[s][p].Next = NULL;

			if (p > 0)
				g_mainGame.Pieces[s][p].Prev = &g_mainGame.Pieces[s][p - 1];
			else
				g_mainGame.Pieces[s][p].Prev = NULL;
		}
	}
}

void PutFreePieceAt(int square, PieceType pieceType, int side01)
{
	for (int p = 0; p < 16; p++)
	{
		Piece *piece = &(g_mainGame.Pieces[side01][p]);
		if (piece->Off && piece->Type == pieceType)
		{
			piece->SquareIndex = square;
			piece->Off = false;
			return;
		}
	}

	if ((pieceType & 7) != KING)
	{
		for (int p = 1; p < 9; p++)
		{
			Piece *piece = &(g_mainGame.Pieces[side01][p]);
			if (piece->Off)
			{
				piece->Type = pieceType;
				piece->SquareIndex = square;
				piece->Off = false;
				return;
			}
		}
	}
}

void InitPiece(int file, int rank, PieceType type, Side color)
{
	g_mainGame.Squares[rank * 8 + file] = type | color;
	PutFreePieceAt(rank * 8 + file, type | color, color >> 4);
}

void InitScores()
{
	g_mainGame.Material[0] = 0;
	g_mainGame.Material[1] = 0;

	for (int s = 0; s < 2; s++)
	{
		for (int p = 0; p < 16; p++)
		{
			Piece piece = g_mainGame.Pieces[s][p];
			if (piece.Off)
				continue;
			PieceType pt = piece.Type & 7;
			int colorSide = (piece.Type & (WHITE | BLACK)) >> 4;
			g_mainGame.Material[colorSide] += MaterialMatrix[colorSide][pt];
		}
	}
}

void InitHash()
{
	g_mainGame.Hash = StartHash;
	for (int s = 0; s < 2; s++)
	{
		for (int p = 0; p < 16; p++)
		{
			Piece piece = g_mainGame.Pieces[s][p];
			if (piece.Off)
				continue;
			int i = piece.SquareIndex;
			g_mainGame.Hash ^= ZobritsPieceTypesSquares[piece.Type][i];
		}
	}
	g_mainGame.Hash ^= ZobritsSides[g_mainGame.Side01];
	if (g_mainGame.State & WhiteCanCastleLong)
		g_mainGame.Hash ^= ZobritsCastlingRights[0];
	if (g_mainGame.State & WhiteCanCastleShort)
		g_mainGame.Hash ^= ZobritsCastlingRights[1];
	if (g_mainGame.State & BlackCanCastleLong)
		g_mainGame.Hash ^= ZobritsCastlingRights[2];
	if (g_mainGame.State & BlackCanCastleShort)
		g_mainGame.Hash ^= ZobritsCastlingRights[3];

	g_mainGame.Hash ^= ZobritsEnpassantFile[g_mainGame.State & 15];
}
