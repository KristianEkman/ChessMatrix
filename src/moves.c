#include <stdio.h>
#include "moves.h"
#include "patterns.h"
#include "evaluation.h"
#include "hashTable.h"
#include "sort.h"

int SetCaptureOff(Game* game, int side, int squareIndex) {
	for (int i = 0; i < 16; i++)
	{
		Piece* capture = &game->Pieces[side][i];
		if (capture->SquareIndex == squareIndex && !capture->Off) {
			capture->Off = true;
			return i;
		}
	}

	printf("Invalid SetCaptureOff parameters\n");
}

void MovePiece(Game* game, int side01, int from, int to) {
	for (int i = 0; i < 16; i++)
	{
		Piece* capture = &game->Pieces[side01][i];
		if (capture->SquareIndex == from && !capture->Off) {
			capture->SquareIndex = to;
			return;
		}
	}
	printf("Invalid MovePiece parameters\n");
}

void AssertGame(Game* game) {
#ifdef _DEBUG
	for (size_t s = 0; s < 2; s++)
	{
		for (size_t p = 0; p < 16; p++)
		{
			Piece* piece = &game->Pieces[s][p];
			PieceType squareType = game->Squares[piece->SquareIndex];

			if (!piece->Off && squareType != piece->Type) {
				printf("Invalid game\n");
			}
		}
	}

	for (size_t i = 0; i < 64; i++)
	{
		PieceType pt = game->Squares[i];
		int side01 = (pt & 24) >> 4;
		bool found = false;
		for (size_t p = 0; p < 16; p++)
		{
			Piece* piece = &game->Pieces[side01][p];
			if (piece->Type == pt && !piece->Off) {
				found = true;
			}
		}
		if (pt != NOPIECE && !found) {
			printf("Invalid game. Square piec not found.\n");
		}
	}
#endif // _DEBUG
}

int MakeMove(Move move, Game* game) {
	char f = move.From;
	char t = move.To;

	PieceType captType = game->Squares[t];
	int captColor = captType >> 4;
	int side01 = game->Side01;

	//removing piece from square removes its position score
	game->PositionScore -= PositionValueMatrix[captType & 7][captColor][t];

	PieceType pieceType = game->Squares[f];

	char pt = pieceType & 7;
	game->PositionScore -= PositionValueMatrix[pt][side01][f];
	game->PositionScore += PositionValueMatrix[pt][side01][t];

	game->Squares[t] = game->Squares[f];
	game->Squares[f] = NOPIECE;

	int captIndex = -1;
	if (captType && move.MoveInfo != EnPassantCapture)
	{
		captIndex = SetCaptureOff(game, !side01, t);
		game->Material[captColor] -= MaterialMatrix[captColor][captType & 7];
	}
	game->Pieces[side01][move.PieceIdx].SquareIndex = t;

	U64 hash = game->Hash;
	hash ^= ZobritsPieceTypesSquares[pieceType][f];
	hash ^= ZobritsPieceTypesSquares[pieceType][t];
	hash ^= ZobritsPieceTypesSquares[captType][t];

	hash ^= ZobritsEnpassantFile[game->State & 15];
	//resetting en passant every move
	game->State &= ~15;

	switch (move.MoveInfo)
	{
	case PromotionQueen:
		game->Squares[t] = QUEEN | game->Side;
		game->Material[side01] += MaterialMatrix[side01][QUEEN + 6];
		hash ^= ZobritsPieceTypesSquares[QUEEN | game->Side][t];
		game->Pieces[side01][move.PieceIdx].Type = (QUEEN | game->Side);

		break;
	case PromotionRook:
		game->Squares[t] = ROOK | game->Side;
		game->Material[side01] += MaterialMatrix[side01][ROOK + 6];
		hash ^= ZobritsPieceTypesSquares[ROOK | game->Side][t];
		game->Pieces[side01][move.PieceIdx].Type = (ROOK | game->Side);

		break;
	case PromotionBishop:
		game->Squares[t] = BISHOP | game->Side;
		game->Material[side01] += MaterialMatrix[side01][BISHOP + 6];
		hash ^= ZobritsPieceTypesSquares[BISHOP | game->Side][t];
		game->Pieces[side01][move.PieceIdx].Type = (BISHOP | game->Side);

		break;
	case PromotionKnight:
		game->Squares[move.To] = KNIGHT | game->Side;
		game->Material[side01] += MaterialMatrix[side01][KNIGHT + 6];
		hash ^= ZobritsPieceTypesSquares[KNIGHT | game->Side][t];
		game->Pieces[side01][move.PieceIdx].Type = (KNIGHT | game->Side);

		break;
	case KingMove:
		game->KingSquares[side01] = t;
		if (game->Side == WHITE)
		{
			if (game->State & WhiteCanCastleLong)
				hash ^= ZobritsCastlingRights[0];
			if (game->State & WhiteCanCastleShort)
				hash ^= ZobritsCastlingRights[1];
		}
		else { //black
			if (game->State & BlackCanCastleLong)
				hash ^= ZobritsCastlingRights[2];
			if (game->State & BlackCanCastleShort)
				hash ^= ZobritsCastlingRights[3];
		}
		game->State &= ~SideCastlingRights[side01]; //sets castling rights bits for current player.

		KingPositionScore(move, game);
		break;
	case RookMove:
		switch (move.From)
		{
		case 0:
			if (game->State & WhiteCanCastleLong) {
				game->State &= ~WhiteCanCastleLong;
				hash ^= ZobritsCastlingRights[0];
			}
			break;
		case 7:
			if (game->State & WhiteCanCastleShort) {
				game->State &= ~WhiteCanCastleShort;
				hash ^= ZobritsCastlingRights[1];
			}
			break;
		case 56:
			if (game->State & BlackCanCastleLong) {
				game->State &= ~BlackCanCastleLong;
				hash ^= ZobritsCastlingRights[2];
			}
			break;
		case 63:
			if (game->State & BlackCanCastleShort) {
				game->State &= ~BlackCanCastleShort;
				hash ^= ZobritsCastlingRights[3];
			}
			break;
		default:
			break;
		}
		break;
	case CastleShort:
	{
		game->KingSquares[side01] = t;
		char rookFr = 7 + CastlesOffset[side01];
		char rookTo = 5 + CastlesOffset[side01];
		PieceType rook = ROOK | game->Side;
		game->Squares[rookFr] = NOPIECE;
		game->Squares[rookTo] = rook;
		MovePiece(game, side01, rookFr, rookTo);

		game->PositionScore += CastlingPoints[side01];
		KingPositionScore(move, game);
		hash ^= ZobritsPieceTypesSquares[rook][rookFr];
		hash ^= ZobritsPieceTypesSquares[rook][rookTo];
		game->State &= ~SideCastlingRights[side01]; //sets castling rights bits for current player.
		hash ^= ZobritsCastlingRights[side01 * 2];
		hash ^= ZobritsCastlingRights[side01 * 2 + 1];
	}
	break;
	case CastleLong:
	{
		game->KingSquares[side01] = t;
		char rookFr = CastlesOffset[side01];
		char rookTo = 3 + CastlesOffset[side01];
		PieceType rook = ROOK | game->Side;
		game->Squares[rookFr] = NOPIECE;
		game->Squares[rookTo] = ROOK | game->Side;
		MovePiece(game, side01, rookFr, rookTo);

		game->PositionScore += CastlingPoints[side01];
		KingPositionScore(move, game);
		hash ^= ZobritsPieceTypesSquares[rook][rookFr];
		hash ^= ZobritsPieceTypesSquares[rook][rookTo];
		game->State &= ~SideCastlingRights[side01]; //sets castling rights bits for current player.
		hash ^= ZobritsCastlingRights[side01 * 2];
		hash ^= ZobritsCastlingRights[side01 * 2 + 1];
	}
	break;
	case EnPassant:
		game->State |= ((f & 7) + 1); //Sets the file. a to h. File is 1 to 8.
		hash ^= ZobritsEnpassantFile[(f & 7) + 1];
		break;
	case EnPassantCapture:
	{
		char behind = t + Behind[side01];
		game->Squares[behind] = NOPIECE;
		captIndex = SetCaptureOff(game, !side01, behind);
		game->Material[side01] += MaterialMatrix[side01][PAWN];
		hash ^= ZobritsPieceTypesSquares[PAWN | captColor][behind];
	}
	break;
	default:
		break;
	}

	hash ^= ZobritsSides[side01];
	game->Hash = hash;
	game->Side ^= 24;
	game->Side01 = game->Side >> 4;
	game->PositionHistory[game->PositionHistoryLength++] = game->Hash;
	AssertGame(game);
	return captIndex;
}

void UnMakeMove(Move move, int captIndex, GameState prevGameState, short prevPositionScore, Game* game, U64 prevHash) {

	int otherSide = game->Side ^ 24;
	int otherSide01 = otherSide >> 4;

	PieceType capture = NOPIECE;
	if (captIndex != -1) {
		capture = game->Pieces[!otherSide01][captIndex].Type;
	}
	game->Material[capture >> 4] += MaterialMatrix[capture >> 4][capture & 7];

	game->Squares[move.From] = game->Squares[move.To];
	if (move.MoveInfo != EnPassantCapture)
		game->Squares[move.To] = capture;

	game->Pieces[otherSide01][move.PieceIdx].SquareIndex = move.From;
	if (capture)
		game->Pieces[!otherSide01][captIndex].Off = false;

	switch (move.MoveInfo)
	{
	case PromotionQueen:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][QUEEN + 6];
		game->Squares[move.From] = PAWN | otherSide;
		game->Pieces[otherSide01][move.PieceIdx].Type = (PAWN | otherSide);
		break;
	case PromotionRook:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][ROOK + 6];
		game->Squares[move.From] = PAWN | otherSide;
		game->Pieces[otherSide01][move.PieceIdx].Type = (PAWN | otherSide);

		break;
	case PromotionBishop:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][BISHOP + 6];
		game->Squares[move.From] = PAWN | otherSide;
		game->Pieces[otherSide01][move.PieceIdx].Type = (PAWN | otherSide);
		break;
	case PromotionKnight:
		game->Material[otherSide01] -= MaterialMatrix[otherSide01][KNIGHT + 6];
		game->Squares[move.From] = PAWN | otherSide;
		game->Pieces[otherSide01][move.PieceIdx].Type = (PAWN | otherSide);
		break;
	case KingMove:
		game->KingSquares[otherSide01] = move.From;
		break;
	case CastleShort:
		game->KingSquares[otherSide01] = move.From;
		game->Squares[5 + CastlesOffset[otherSide01]] = NOPIECE;
		game->Squares[7 + CastlesOffset[otherSide01]] = ROOK | otherSide;
		MovePiece(game, otherSide01, 5 + CastlesOffset[otherSide01], 7 + CastlesOffset[otherSide01]);
		break;
	case CastleLong:
		game->KingSquares[otherSide01] = move.From;
		game->Squares[3 + CastlesOffset[otherSide01]] = NOPIECE;
		game->Squares[0 + CastlesOffset[otherSide01]] = ROOK | otherSide;
		MovePiece(game, otherSide01, 3 + CastlesOffset[otherSide01], 0 + CastlesOffset[otherSide01]);
		break;
	case EnPassantCapture:
		game->Squares[move.To + Behind[otherSide01]] = PAWN | game->Side;
		game->Squares[move.To] = NOPIECE;
		// captured piece should be put back earlier
		break;
	default:
		break;
	}
	game->State = prevGameState;
	game->PositionScore = prevPositionScore;
	game->Hash = prevHash;
	game->Side ^= 24;
	game->Side01 = game->Side >> 4;
	game->PositionHistoryLength--;
	AssertGame(game);
}

void MakeNullMove(Game* game) {
	int side01 = game->Side01;
	U64 hash = ZobritsEnpassantFile[game->State & 15];
	//resetting en passant
	game->State &= ~15;

	hash ^= ZobritsSides[side01];
	game->Hash ^= hash;
	game->Side ^= 24;
	game->Side01 = game->Side >> 4;
	game->PositionHistory[game->PositionHistoryLength++] = game->Hash;
}

void UnMakeNullMove(GameState prevGameState, Game* game, U64 prevHash) {
	int otherSide = game->Side ^ 24;
	int otherSide01 = otherSide >> 4;
	game->State = prevGameState;
	game->Hash = prevHash;
	game->Side ^= 24;
	game->Side01 = game->Side >> 4;
	game->PositionHistoryLength--;
}

bool SquareAttacked(int square, Side attackedBy, Game* game) {
	int side01 = attackedBy >> 4;
	for (size_t pi = 0; pi < 16; pi++)
	{
		Piece* piece = &game->Pieces[side01][pi];
		if (piece->Off)
			continue;
		int i = piece->SquareIndex;
		PieceType pieceType = game->Squares[i];
		PieceType pt = pieceType & 7;
		switch (pt)
		{
		case PAWN:
		{
			int captPat = PawnCapturePattern[side01];
			int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
			for (int pc = 1; pc <= pawnCapPatLength; pc++)
			{
				int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
				if (toSquare == square)
					return true;
			}
			break;
		}
		case KNIGHT:
		{
			int length = PieceTypeSquarePatterns[0][i][0];
			for (int p = 1; p <= length; p++)
			{
				int toSquare = PieceTypeSquarePatterns[0][i][p];
				if (toSquare == square)
					return true;
			}
			break;
		}
		case KING:
		{
			int length = PieceTypeSquarePatterns[1][i][0];
			for (int p = 1; p <= length; p++)
			{
				int toSquare = PieceTypeSquarePatterns[1][i][p];
				if (toSquare == square)
					return true;
			}
			break;
		}
		default:
		{
			int pat = pt - 1;
			char raysCount = PieceTypeSquareRaysPatterns[pat][i][0][0];
			for (int r = 1; r <= raysCount; r++)
			{
				int rayLength = PieceTypeSquareRaysPatterns[pat][i][r][0];
				for (int rr = 1; rr <= rayLength; rr++)
				{
					int toSquare = PieceTypeSquareRaysPatterns[pat][i][r][rr];
					if (toSquare == square)
						return true;
					if (game->Squares[toSquare] > NOPIECE)
						break;
				}
			}
			break;
		}
		}
	}
	return false;
}

void SortMoves(Move* moves, int moveCount, Side side) {

	if (side == WHITE)
		QuickSort(moves, 0, moveCount - 1);
	else
		QuickSortDescending(moves, 0, moveCount - 1);
}

void CreateMove(int fromSquare, int toSquare, MoveInfo moveInfo, Game* game, char pieceIdx) {
	GameState prevGameState = game->State;
	Move move;
	move.From = fromSquare;
	move.To = toSquare;
	move.MoveInfo = moveInfo;
	move.PieceIdx = pieceIdx;
	short prevPosScore = game->PositionScore;
	U64 prevHash = game->Hash;

	int captIndex = MakeMove(move, game);
	move.ScoreAtDepth = GetMoveScore(game);
	//move.ScoreAtDepth = GetEval(game, move.ScoreAtDepth);
	game->MovesBuffer[game->MovesBufferLength++] = move;

	UnMakeMove(move, captIndex, prevGameState, prevPosScore, game, prevHash);
}

void CreateMoves(Game* game, int depth) {
	game->MovesBufferLength = 0;
	for (size_t pi = 0; pi < 16; pi++)
	{
		Piece* piece = &game->Pieces[game->Side01][pi];
		if (piece->Off)
			continue;
		int i = piece->SquareIndex;
		PieceType pieceType = game->Squares[i];
		PieceType pt = pieceType & 7;
		switch (pt)
		{
		case PAWN:
		{
			int pat = PawnPattern[game->Side01];
			int pawnPatLength = PieceTypeSquarePatterns[pat][i][0];
			for (int pp = 1; pp <= pawnPatLength; pp++)
			{
				int toSquare = PieceTypeSquarePatterns[pat][i][pp];
				if (game->Squares[toSquare] != NOPIECE)
					break;
				if (toSquare < 8 || toSquare > 55) {
					CreateMove(i, toSquare, PromotionQueen, game, pi);
					CreateMove(i, toSquare, PromotionRook, game, pi);
					CreateMove(i, toSquare, PromotionBishop, game, pi);
					CreateMove(i, toSquare, PromotionKnight, game, pi);
				}
				else if (pp == 2) {
					CreateMove(i, toSquare, EnPassant, game, pi);
				}
				else {
					CreateMove(i, toSquare, PlainMove, game, pi);
				}
			}

			int captPat = PawnCapturePattern[game->Side01];
			int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
			for (int pc = 1; pc <= pawnCapPatLength; pc++)
			{
				int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
				//Must be a piece of opposite color.
				if (game->Squares[toSquare] & (game->Side ^ 24))
				{
					if (toSquare < 8 || toSquare > 55) {
						CreateMove(i, toSquare, PromotionQueen, game, pi);
						CreateMove(i, toSquare, PromotionRook, game, pi);
						CreateMove(i, toSquare, PromotionBishop, game, pi);
						CreateMove(i, toSquare, PromotionKnight, game, pi);
					}
					else {
						CreateMove(i, toSquare, PlainMove, game, pi);
					}
				}
				else {
					int enpFile = (game->State & 15) - 1;
					if (enpFile > -1) {
						int toFile = toSquare & 7;
						int toRank = toSquare >> 3;
						if (toFile == enpFile && toRank == EnpassantRankPattern[game->Side01])
							CreateMove(i, toSquare, EnPassantCapture, game, pi);
					}
				}
			}
			break;
		}
		case KNIGHT:
		{
			int length = PieceTypeSquarePatterns[0][i][0];
			for (int p = 1; p <= length; p++)
			{
				int toSquare = PieceTypeSquarePatterns[0][i][p];
				if (!(game->Squares[toSquare] & game->Side)) {
					CreateMove(i, toSquare, 0, game, pi);
				}
			}
			break;
		}
		case KING:
		{
			int length = PieceTypeSquarePatterns[1][i][0];
			for (int p = 1; p <= length; p++)
			{
				int toSquare = PieceTypeSquarePatterns[1][i][p];
				if (!(game->Squares[toSquare] & game->Side)) {
					CreateMove(i, toSquare, KingMove, game, pi);
				}
			}

			int castleBlackOffset = CastlesOffset[game->Side01];
			if (i == castleBlackOffset + 4) { //King on origin pos
				if ((game->Side & WHITE && game->State & WhiteCanCastleShort) || (game->Side & BLACK && game->State & BlackCanCastleShort)) {
					if ((game->Squares[castleBlackOffset + 7] & 7) == ROOK &&
						game->Squares[castleBlackOffset + 5] == NOPIECE &&
						game->Squares[castleBlackOffset + 6] == NOPIECE)
					{
						if (!SquareAttacked(5 + castleBlackOffset, game->Side ^ 24, game) && !SquareAttacked(4 + castleBlackOffset, game->Side ^ 24, game))
							CreateMove(i, 6 + castleBlackOffset, CastleShort, game, pi);
					}
				}
				if ((game->Side & WHITE && game->State & WhiteCanCastleLong) || (game->Side & BLACK && game->State & BlackCanCastleLong)) {
					if ((game->Squares[castleBlackOffset] & 7) == ROOK &&
						game->Squares[castleBlackOffset + 1] == NOPIECE &&
						game->Squares[castleBlackOffset + 2] == NOPIECE &&
						game->Squares[castleBlackOffset + 3] == NOPIECE)
					{
						if (!SquareAttacked(4 + castleBlackOffset, game->Side ^ 24, game) && !SquareAttacked(3 + castleBlackOffset, game->Side ^ 24, game))
							CreateMove(i, 2 + castleBlackOffset, CastleLong, game, pi);
					}
				}
			}
			break;
		}
		default:
		{
			int pat = pt - 1;
			int raysCount = PieceTypeSquareRaysPatterns[pat][i][0][0];
			for (int r = 1; r <= raysCount; r++)
			{
				int rayLength = PieceTypeSquareRaysPatterns[pat][i][r][0];
				for (int rnd_seed = 1; rnd_seed <= rayLength; rnd_seed++)
				{
					int toSquare = PieceTypeSquareRaysPatterns[pat][i][r][rnd_seed];
					PieceType toPiece = game->Squares[toSquare];
					MoveInfo moveInfo = pt == ROOK ? RookMove : PlainMove;

					if (toPiece != NOPIECE) {
						if (!(toPiece & game->Side)) {
							CreateMove(i, toSquare, moveInfo, game, pi);
						}
						break;
					}
					else {
						CreateMove(i, toSquare, moveInfo, game, pi);
					}
				}
			}
			break;
		}
		}

	}
	SortMoves(game->MovesBuffer, game->MovesBufferLength, game->Side);
}

void CreateCaptureMoves(Game* game) {
	game->MovesBufferLength = 0;
	int side01 = game->Side >> 4;
	char otherSide = game->Side ^ 24;

	for (size_t pi = 0; pi < 16; pi++)
	{
		Piece* piece = &game->Pieces[side01][pi];
		if (piece->Off)
			continue;
		int i = piece->SquareIndex;

		PieceType pieceType = game->Squares[i];
		PieceType pt = pieceType & 7;
		switch (pt)
		{
		case PAWN:
		{
			int infront = -Behind[side01];
			int toSquare = i + infront;
			if (game->Squares[toSquare] == NOPIECE)
			{
				if (toSquare < 8 || toSquare > 55) {
					CreateMove(i, toSquare, PromotionQueen, game, pi);
					/*CreateMove(i, toSquare, PromotionRook, game, pi);
					CreateMove(i, toSquare, PromotionBishop, game, pi);
					CreateMove(i, toSquare, PromotionKnight, game, pi);*/
				}
			}

			int captPat = PawnCapturePattern[side01];
			int pawnCapPatLength = PieceTypeSquarePatterns[captPat][i][0];
			for (int pc = 1; pc <= pawnCapPatLength; pc++)
			{
				int toSquare = PieceTypeSquarePatterns[captPat][i][pc];
				//Must be a piece of opposite color.
				if (game->Squares[toSquare] & otherSide)
				{
					if (toSquare < 8 || toSquare > 55) {
						CreateMove(i, toSquare, PromotionQueen, game, pi);
						/*CreateMove(i, toSquare, PromotionRook, game, pi);
						CreateMove(i, toSquare, PromotionBishop, game, pi);
						CreateMove(i, toSquare, PromotionKnight, game, pi);*/
					}
					else {
						CreateMove(i, toSquare, PlainMove, game, pi);
					}
				}
				else {
					int enpFile = (game->State & 15) - 1;
					if (enpFile > -1) {
						int toFile = toSquare & 7;
						int toRank = toSquare >> 3;
						if (toFile == enpFile && toRank == EnpassantRankPattern[side01])
							CreateMove(i, toSquare, EnPassantCapture, game, pi);
					}
				}
			}
			break;
		}
		case KNIGHT:
		{
			int length = PieceTypeSquarePatterns[0][i][0];
			for (int p = 1; p <= length; p++)
			{
				int toSquare = PieceTypeSquarePatterns[0][i][p];
				if (game->Squares[toSquare] & otherSide) {
					CreateMove(i, toSquare, 0, game, pi);
				}
			}
			break;
		}
		case KING:
		{
			int length = PieceTypeSquarePatterns[1][i][0];
			for (int p = 1; p <= length; p++)
			{
				int toSquare = PieceTypeSquarePatterns[1][i][p];
				if (game->Squares[toSquare] & otherSide) {
					CreateMove(i, toSquare, KingMove, game, pi);
				}
			}
			break;
		}
		default:
		{
			int pat = pt - 1;
			int raysCount = PieceTypeSquareRaysPatterns[pat][i][0][0];
			for (int r = 1; r <= raysCount; r++)
			{
				int rayLength = PieceTypeSquareRaysPatterns[pat][i][r][0];
				for (int rr = 1; rr <= rayLength; rr++)
				{
					int toSquare = PieceTypeSquareRaysPatterns[pat][i][r][rr];
					PieceType toPiece = game->Squares[toSquare];
					MoveInfo moveInfo = pt == ROOK ? RookMove : PlainMove;
					if (toPiece & otherSide) {
						CreateMove(i, toSquare, moveInfo, game, pi);
						break;
					}
				}
			}
			break;
		}
		}
	}
	SortMoves(game->MovesBuffer, game->MovesBufferLength, game->Side);
}

void RemoveInvalidMoves(Game* game) {
	int validMovesCount = 0;
	Move validMoves[100];

	for (size_t m = 0; m < game->MovesBufferLength; m++)
	{
		Move move = game->MovesBuffer[m];
		GameState prevState = game->State;
		short prevPosScor = game->PositionScore;
		U64 prevHash = game->Hash;
		int captIndex = MakeMove(move, game);
		int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];

		bool legal = !SquareAttacked(kingSquare, game->Side, game);
		UnMakeMove(move, captIndex, prevState, prevPosScor, game, prevHash);
		if (legal)
			validMoves[validMovesCount++] = move;
	}
	memcpy(game->MovesBuffer, &validMoves, validMovesCount * sizeof(Move));
	game->MovesBufferLength = validMovesCount;
}

int ValidMoves(Move* moves) {
	CreateMoves(&g_mainGame, 0);
	RemoveInvalidMoves(&g_mainGame);

	if (g_mainGame.MovesBufferLength == 0)
		return 0;

	memcpy(moves, g_mainGame.MovesBuffer, g_mainGame.MovesBufferLength * sizeof(Move));
	return g_mainGame.MovesBufferLength;
}

int ValidMovesOnThread(Game* game, Move* moves) {
	CreateMoves(game, 0);
	RemoveInvalidMoves(game);

	if (game->MovesBufferLength == 0)
		return 0;

	memcpy(moves, game->MovesBuffer, game->MovesBufferLength * sizeof(Move));
	return game->MovesBufferLength;
}

Move ParseMove(char* sMove, MoveInfo info) {
	int fromFile = sMove[0] - 'a';
	int fromRank = sMove[1] - '1';
	int toFile = sMove[2] - 'a';
	int toRank = sMove[3] - '1';
	Move move;
	move.From = fromRank * 8 + fromFile;
	move.To = toRank * 8 + toFile;
	move.MoveInfo = info;
	return move;
}

PlayerMove MakePlayerMoveOnThread(Game* game, char* sMove) {
	Move move = ParseMove(sMove, 0);
	Move validMoves[256];
	int length = ValidMovesOnThread(game, validMoves);
	PlayerMove playerMove;
	for (int i = 0; i < length; i++)
	{
		if (validMoves[i].From == move.From && validMoves[i].To == move.To) {
			playerMove.Move = validMoves[i];
			playerMove.PreviousGameState = game->State;
			playerMove.Invalid = false;
			playerMove.PreviousPositionScore = game->PositionScore;
			playerMove.PreviousHash = game->Hash;

			int captIndex = MakeMove(validMoves[i], game);
			playerMove.CaptureIndex = captIndex;
			return playerMove;
		}
	}
	playerMove.Invalid = true;
	return playerMove;
}

PlayerMove MakePlayerMove(char* sMove) {
	return MakePlayerMoveOnThread(&g_mainGame, sMove);
}

void UnMakePlayerMove(PlayerMove playerMove) {
	UnMakeMove(playerMove.Move, playerMove.CaptureIndex, playerMove.PreviousGameState, playerMove.PreviousPositionScore, &g_mainGame, playerMove.PreviousHash);
}

void UnMakePlayerMoveOnThread(Game* game, PlayerMove playerMove) {
	UnMakeMove(playerMove.Move, playerMove.CaptureIndex, playerMove.PreviousGameState, playerMove.PreviousPositionScore, game, playerMove.PreviousHash);
}


void MoveToString(Move move, char* sMove) {
	char fromFile = (move.From & 7) + 'a';
	char fromRank = (move.From >> 3) + '1';
	char toFile = (move.To & 7) + 'a';
	char toRank = (move.To >> 3) + '1';
	sMove[0] = fromFile;
	sMove[1] = fromRank;
	sMove[2] = toFile;
	sMove[3] = toRank;
	sMove[4] = '\0';
}