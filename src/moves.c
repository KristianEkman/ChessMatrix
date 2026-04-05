#include <stdio.h>
#include "moves.h"
#include "bitboards.h"
#include "patterns.h"
#include "evaluation.h"
#include "hashTable.h"
#include "position.h"
#include "countermoves.h"
#include <string.h>

static void ClearCastlingRightsForRookSquare(Game *game, int square, U64 *hash)
{
	switch (square)
	{
	case 0:
		if (game->State & WhiteCanCastleLong)
		{
			game->State &= ~WhiteCanCastleLong;
			*hash ^= ZobritsCastlingRights[0];
		}
		break;
	case 7:
		if (game->State & WhiteCanCastleShort)
		{
			game->State &= ~WhiteCanCastleShort;
			*hash ^= ZobritsCastlingRights[1];
		}
		break;
	case 56:
		if (game->State & BlackCanCastleLong)
		{
			game->State &= ~BlackCanCastleLong;
			*hash ^= ZobritsCastlingRights[2];
		}
		break;
	case 63:
		if (game->State & BlackCanCastleShort)
		{
			game->State &= ~BlackCanCastleShort;
			*hash ^= ZobritsCastlingRights[3];
		}
		break;
	default:
		break;
	}
}

static PieceType GetPromotionPiece(MoveInfo moveInfo, Side side)
{
	switch (moveInfo)
	{
	case PromotionQueen:
		return QUEEN | side;
	case PromotionRook:
		return ROOK | side;
	case PromotionBishop:
		return BISHOP | side;
	case PromotionKnight:
		return KNIGHT | side;
	default:
		return NOPIECE;
	}
}

static bool IsPromotionSquare(int square)
{
	return square < 8 || square > 55;
}

static bool IsEnPassantTargetSquare(Game *game, int square)
{
	int enPassantFile = (game->State & 15) - 1;
	if (enPassantFile < 0)
		return false;

	return (square & 7) == enPassantFile && (square >> 3) == EnpassantRankPattern[game->Side01];
}

static void ApplyPromotion(Game *game, Move move, int side01, PieceType pawnPiece, PieceType promotedPiece, U64 *hash, AllPieceBitboards *bitboards)
{
	int toSquare = move.To;
	game->Squares[toSquare] = promotedPiece;
	game->Material[side01] += MaterialMatrix[side01][(promotedPiece & 7) + 6];
	*hash ^= ZobritsPieceTypesSquares[pawnPiece][toSquare];
	*hash ^= ZobritsPieceTypesSquares[promotedPiece][toSquare];
	game->Pieces[side01][move.PieceIdx].Type = promotedPiece;
	ReplacePieceOnBitboards(bitboards, pawnPiece, promotedPiece, toSquare);
}

static void UndoPromotion(Game *game, Move move, int side01, Side side, PieceType promotedPiece)
{
	game->Material[side01] -= MaterialMatrix[side01][(promotedPiece & 7) + 6];
	game->Squares[move.From] = PAWN | side;
	game->Pieces[side01][move.PieceIdx].Type = (PAWN | side);
}

static void SetCaptureOff(Game *game, int side, int squareIndex, Undos *undos)
{
	Piece *capture = &game->Pieces[side][0];
	while (capture)
	{
		if (capture->SquareIndex == squareIndex)
		{
			capture->Off = true;
			Piece *prev = capture->Prev; // King is first so it will always be on the board.
			Piece *next = capture->Next;

			undos->CaptIndex = capture->Index;
			if (prev)
				prev->Next = next; // jumping over the capture.

			if (next)
				next->Prev = prev; // skipping it both directions in the linked list

			return;
		}
		capture = capture->Next;
	}

	printf("Invalid SetCaptureOff parameters\n");
	return;
}

static void MovePiece(Game *game, int side01, int from, int to)
{
	Piece *piece = &game->Pieces[side01][0];
	while (piece != NULL)
	{
		if (piece->SquareIndex == from)
		{
			piece->SquareIndex = to;
			return;
		}
		piece = piece->Next;
	}
	printf("Invalid MovePiece parameters\n");
}

void AssertGame(Game *game)
{
	(void)game;
#ifdef _DEBUG
	for (int s = 0; s < 2; s++)
	{
		for (int p = 0; p < 16; p++)
		{
			Piece *piece = &game->Pieces[s][p];
			PieceType squareType = game->Squares[piece->SquareIndex];

			if (!piece->Off && squareType != piece->Type)
			{
				printf("Invalid game. piece Type and square type not equal.\n");
			}

			Piece *next = piece->Next;
			if (next && !piece->Off && next->Off)
				printf("Invalid game. Next piece sould not be Off.\n");
		}
	}

	for (int i = 0; i < 64; i++)
	{
		PieceType pt = game->Squares[i];
		int side01 = (pt & 24) >> 4;
		bool found = false;
		for (int p = 0; p < 16; p++)
		{
			Piece *piece = &game->Pieces[side01][p];
			if (piece->Type == pt && !piece->Off)
			{
				found = true;
			}
		}
		if (pt != NOPIECE && !found)
		{
			printf("Invalid game. Square piece not found.\n");
		}
	}
#endif // _DEBUG
}

short KingPositionScore(Move move, Game *game)
{
	return GetKingPositionScore(move, game);
}

Undos DoMove(Move move, Game *game)
{
	Undos undos;
	undos.PrevGameState = game->State;
	undos.PrevHash = game->Hash;
	undos.CaptIndex = -1;
	undos.PositionHistoryPushed = false;

	int f = move.From;
	int t = move.To;

	PieceType captType = game->Squares[t];
	int captColor = captType >> 4;
	int side01 = game->Side01;

	// removing piece from square removes its position score

	PieceType pieceType = game->Squares[f];
	PieceType pt = pieceType & 7;
	U64 hash = game->Hash;
	uchar oldHashEnPassantFile = GetHashEnPassantFile(game);
	uchar newHashEnPassantFile = 0;
	AllPieceBitboards *bitboards = &game->Bitboards;
	PieceType promotedPiece = GetPromotionPiece(move.MoveInfo, game->Side);

	if (captType && move.MoveInfo != EnPassantCapture)
		RemovePieceFromBitboards(bitboards, captType, t);
	MovePieceOnBitboards(bitboards, pieceType, f, t);
	game->Squares[t] = game->Squares[f];
	game->Squares[f] = NOPIECE;

	// int captIndex = -1;
	if (captType && move.MoveInfo != EnPassantCapture)
	{
		SetCaptureOff(game, !side01, t, &undos);
		game->Material[captColor] -= MaterialMatrix[captColor][captType & 7];
		if ((captType & 7) == ROOK)
			ClearCastlingRightsForRookSquare(game, t, &hash);
	}
	game->Pieces[side01][move.PieceIdx].SquareIndex = t;
	game->Pieces[side01][move.PieceIdx].MoveCount++;
	hash ^= ZobritsPieceTypesSquares[pieceType][f];
	hash ^= ZobritsPieceTypesSquares[pieceType][t];
	hash ^= ZobritsPieceTypesSquares[captType][t];

	hash ^= ZobritsEnpassantFile[oldHashEnPassantFile];
	// resetting en passant every move
	game->State &= ~15;

	switch (move.MoveInfo)
	{
	case KingMove:
		game->KingSquares[side01] = t;
		if (game->Side == WHITE)
		{
			if (game->State & WhiteCanCastleLong)
				hash ^= ZobritsCastlingRights[0];
			if (game->State & WhiteCanCastleShort)
				hash ^= ZobritsCastlingRights[1];
		}
		else
		{ // black
			if (game->State & BlackCanCastleLong)
				hash ^= ZobritsCastlingRights[2];
			if (game->State & BlackCanCastleShort)
				hash ^= ZobritsCastlingRights[3];
		}
		game->State &= ~SideCastlingRights[side01]; // sets castling rights bits for current player.
		break;
	case RookMove:
		ClearCastlingRightsForRookSquare(game, move.From, &hash);
		break;
	case CastleShort:
	{
		game->KingSquares[side01] = t;
		int rookFr = 7 + CastlesOffset[side01];
		int rookTo = 5 + CastlesOffset[side01];
		PieceType rook = ROOK | game->Side;
		game->Squares[rookFr] = NOPIECE;
		game->Squares[rookTo] = rook;
		MovePiece(game, side01, rookFr, rookTo);
		MovePieceOnBitboards(bitboards, rook, rookFr, rookTo);
		hash ^= ZobritsPieceTypesSquares[rook][rookFr];
		hash ^= ZobritsPieceTypesSquares[rook][rookTo];

		// g�r inte detta om det redan �r gjort
		if ((game->Side == WHITE && (game->State & WhiteCanCastleLong)) || (game->Side == BLACK && (game->State & BlackCanCastleLong)))
			hash ^= ZobritsCastlingRights[side01 * 2]; // 0 eller 2, white long, black long

		hash ^= ZobritsCastlingRights[side01 * 2 + 1]; // 1 eller 3, white short, black short
		game->State &= ~SideCastlingRights[side01];	   // sets castling rights bits for current player.
	}
	break;
	case CastleLong:
	{
		game->KingSquares[side01] = t;
		int rookFr = CastlesOffset[side01];
		int rookTo = 3 + CastlesOffset[side01];
		PieceType rook = ROOK | game->Side;
		game->Squares[rookFr] = NOPIECE;
		game->Squares[rookTo] = ROOK | game->Side;
		MovePiece(game, side01, rookFr, rookTo);
		MovePieceOnBitboards(bitboards, rook, rookFr, rookTo);
		KingPositionScore(move, game);
		hash ^= ZobritsPieceTypesSquares[rook][rookFr];
		hash ^= ZobritsPieceTypesSquares[rook][rookTo];

		hash ^= ZobritsCastlingRights[side01 * 2]; // long
		if ((game->Side == WHITE && game->State & WhiteCanCastleShort) || (game->Side == BLACK && (game->State & BlackCanCastleShort)))
			hash ^= ZobritsCastlingRights[side01 * 2 + 1]; // short

		game->State &= ~SideCastlingRights[side01]; // sets castling rights bits for current player.
	}
	break;
	case EnPassant:
		game->State |= ((f & 7) + 1); // Sets the file. a to h. File is 1 to 8.
		newHashEnPassantFile = GetHashEnPassantFileForState(game, (uchar)((f & 7) + 1), !side01);
		break;
	case EnPassantCapture:
	{
		int behind = t + Behind[side01];
		game->Squares[behind] = NOPIECE;
		SetCaptureOff(game, !side01, behind, &undos);
		game->Material[!side01] -= MaterialMatrix[!side01][PAWN];
		hash ^= ZobritsPieceTypesSquares[PAWN | (game->Side ^ 24)][behind];
		RemovePieceFromBitboards(bitboards, PAWN | (game->Side ^ 24), behind);
	}
	break;
	default:
		break;
	}

	if (promotedPiece != NOPIECE)
		ApplyPromotion(game, move, side01, pieceType, promotedPiece, &hash, bitboards);

	hash ^= ZobritsEnpassantFile[newHashEnPassantFile];
	hash ^= ZobritsSides[side01];
	hash ^= ZobritsSides[!side01];
	game->Hash = hash;
	game->Side ^= 24;
	game->Side01 = game->Side >> 4;
	if (game->PositionHistoryLength < MAX_POSITION_HISTORY)
	{
		game->PositionHistory[game->PositionHistoryLength++] = game->Hash;
		undos.PositionHistoryPushed = true;
	}
	undos.FiftyMoveRuleCount = game->FiftyMoveRuleCount;
	if (pt == PAWN || captType)
		game->FiftyMoveRuleCount = 0;
	else
		game->FiftyMoveRuleCount++;
	AssertGame(game);
	return undos;
}

void UndoMove(Game *game, Move move, Undos undos)
{

	Side otherSide = game->Side ^ 24;
	int otherSide01 = otherSide >> 4;
	AllPieceBitboards *bitboards = &game->Bitboards;
	PieceType movingPiece = game->Squares[move.To];
	PieceType pawnPiece = PAWN | otherSide;
	PieceType promotedPiece = GetPromotionPiece(move.MoveInfo, otherSide);

	PieceType capture = NOPIECE;
	if (undos.CaptIndex != -1)
	{
		capture = game->Pieces[!otherSide01][undos.CaptIndex].Type;
	}
	game->Material[capture >> 4] += MaterialMatrix[capture >> 4][capture & 7];

	if (promotedPiece != NOPIECE)
	{
		RemovePieceFromBitboards(bitboards, movingPiece, move.To);
		AddPieceToBitboards(bitboards, pawnPiece, move.From);
		if (capture)
			AddPieceToBitboards(bitboards, capture, move.To);
	}
	else
	{
		switch (move.MoveInfo)
		{
		case CastleShort:
		{
			PieceType rook = ROOK | otherSide;
			MovePieceOnBitboards(bitboards, movingPiece, move.To, move.From);
			MovePieceOnBitboards(bitboards, rook, 5 + CastlesOffset[otherSide01], 7 + CastlesOffset[otherSide01]);
			break;
		}
		case CastleLong:
		{
			PieceType rook = ROOK | otherSide;
			MovePieceOnBitboards(bitboards, movingPiece, move.To, move.From);
			MovePieceOnBitboards(bitboards, rook, 3 + CastlesOffset[otherSide01], 0 + CastlesOffset[otherSide01]);
			break;
		}
		case EnPassantCapture:
			MovePieceOnBitboards(bitboards, movingPiece, move.To, move.From);
			AddPieceToBitboards(bitboards, PAWN | game->Side, move.To + Behind[otherSide01]);
			break;
		default:
			MovePieceOnBitboards(bitboards, movingPiece, move.To, move.From);
			if (capture)
				AddPieceToBitboards(bitboards, capture, move.To);
			break;
		}
	}

	game->Squares[move.From] = game->Squares[move.To];
	if (move.MoveInfo != EnPassantCapture)
		game->Squares[move.To] = capture;

	game->Pieces[otherSide01][move.PieceIdx].SquareIndex = move.From;
	game->Pieces[otherSide01][move.PieceIdx].MoveCount--;
	if (capture)
	{
		Piece *pCapt = &game->Pieces[!otherSide01][undos.CaptIndex];
		Piece *prev = pCapt->Prev; // these are not changed (hopefully)
		Piece *next = pCapt->Next;
		pCapt->Off = false;

		// restoring the piece links
		if (prev)
			prev->Next = pCapt;
		if (next)
			next->Prev = pCapt;
	}

	if (promotedPiece != NOPIECE)
	{
		UndoPromotion(game, move, otherSide01, otherSide, promotedPiece);
	}
	else
	{
		switch (move.MoveInfo)
		{
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
	}
	game->State = undos.PrevGameState;
	game->Hash = undos.PrevHash;
	game->Side = otherSide;
	game->Side01 = otherSide01;
	if (undos.PositionHistoryPushed && game->PositionHistoryLength > 0)
		game->PositionHistoryLength--;
	game->FiftyMoveRuleCount = undos.FiftyMoveRuleCount;
	AssertGame(game);
}

bool DoNullMove(Game *game)
{
	int side01 = game->Side01;
	U64 hash = ZobritsEnpassantFile[GetHashEnPassantFile(game)];
	// resetting en passant
	game->State &= ~15;

	hash ^= ZobritsSides[side01];
	hash ^= ZobritsSides[!side01];
	game->Hash ^= hash;
	game->Side ^= 24;
	game->Side01 = game->Side >> 4;
	return false;
}

void UndoNullMove(GameState prevGameState, Game *game, U64 prevHash, bool positionHistoryPushed)
{
	game->State = prevGameState;
	game->Hash = prevHash;
	game->Side ^= 24;
	game->Side01 = game->Side >> 4;
	if (positionHistoryPushed && game->PositionHistoryLength > 0)
		game->PositionHistoryLength--;
}

static bool PatternAttackerFound(U64 attackers, int patternIndex, int square)
{
	int length = PieceTypeSquarePatterns[patternIndex][square][0];
	for (int p = 1; p <= length; p++)
	{
		int fromSquare = PieceTypeSquarePatterns[patternIndex][square][p];
		if (attackers & SquareBitUnchecked(fromSquare))
			return true;
	}
	return false;
}

static bool RayAttackerFound(const AllPieceBitboards *bitboards, U64 attackers, int patternIndex, int square)
{
	int raysCount = PieceTypeSquareRaysPatterns[patternIndex][square][0][0];
	for (int r = 1; r <= raysCount; r++)
	{
		int rayLength = PieceTypeSquareRaysPatterns[patternIndex][square][r][0];
		for (int rr = 1; rr <= rayLength; rr++)
		{
			int fromSquare = PieceTypeSquareRaysPatterns[patternIndex][square][r][rr];
			U64 squareBit = SquareBitUnchecked(fromSquare);
			if ((bitboards->Occupied & squareBit) == 0)
				continue;

			if (attackers & squareBit)
				return true;
			break;
		}
	}
	return false;
}

static U64 PatternAttackerSquares(U64 attackers, int patternIndex, int square)
{
	U64 found = 0ULL;
	int length = PieceTypeSquarePatterns[patternIndex][square][0];
	for (int p = 1; p <= length; p++)
	{
		int fromSquare = PieceTypeSquarePatterns[patternIndex][square][p];
		U64 fromBit = SquareBitUnchecked(fromSquare);
		if (attackers & fromBit)
			found |= fromBit;
	}
	return found;
}

void BuildLegalMoveContext(Game *game, LegalMoveContext *ctx)
{
	memset(ctx, 0, sizeof(*ctx));

	int side01 = game->Side01;
	int enemySide01 = !side01;
	int kingSquare = game->KingSquares[side01];
	const AllPieceBitboards *bitboards = &game->Bitboards;
	U64 occupied = bitboards->Occupied;

	ctx->KingSquare = kingSquare;

	U64 knightCheckers = PatternAttackerSquares(bitboards->Matrix[KNIGHT][enemySide01], 0, kingSquare);
	U64 kingCheckers = PatternAttackerSquares(bitboards->Matrix[KING][enemySide01], 1, kingSquare);
	U64 pawnCheckers = PatternAttackerSquares(bitboards->Matrix[PAWN][enemySide01], PawnCapturePattern[side01], kingSquare);

	ctx->Checkers |= knightCheckers | kingCheckers | pawnCheckers;
	ctx->CheckCount += (uchar)popcount(knightCheckers | kingCheckers | pawnCheckers);
	ctx->EvasionMask |= knightCheckers | kingCheckers | pawnCheckers;

	for (int patternIndex = 0; patternIndex < 2; patternIndex++)
	{
		int raysCount = PieceTypeSquareRaysPatterns[patternIndex][kingSquare][0][0];
		for (int r = 1; r <= raysCount; r++)
		{
			int firstFriendlySquare = -1;
			U64 rayMask = 0ULL;
			int rayLength = PieceTypeSquareRaysPatterns[patternIndex][kingSquare][r][0];
			for (int rr = 1; rr <= rayLength; rr++)
			{
				int square = PieceTypeSquareRaysPatterns[patternIndex][kingSquare][r][rr];
				U64 squareBit = SquareBitUnchecked(square);
				rayMask |= squareBit;
				if ((occupied & squareBit) == 0ULL)
					continue;

				PieceType piece = game->Squares[square];
				if (piece & game->Side)
				{
					if (firstFriendlySquare == -1)
					{
						firstFriendlySquare = square;
						continue;
					}
					break;
				}

				PieceType enemyPiece = piece & 7;
				bool compatibleSlider = patternIndex == 0
					? (enemyPiece == BISHOP || enemyPiece == QUEEN)
					: (enemyPiece == ROOK || enemyPiece == QUEEN);
				if (!compatibleSlider)
					break;

				if (firstFriendlySquare == -1)
				{
					ctx->Checkers |= squareBit;
					ctx->CheckCount++;
					ctx->EvasionMask |= rayMask;
				}
				else
				{
					U64 pinnedBit = SquareBitUnchecked(firstFriendlySquare);
					ctx->Pinned |= pinnedBit;
					ctx->PinLineMasks[firstFriendlySquare] = rayMask;
				}
				break;
			}
		}
	}

	if (ctx->CheckCount == 0)
		ctx->EvasionMask = ~0ULL;
}

FastMoveLegality ClassifyMoveLegality(Move move, Game *game, const LegalMoveContext *ctx)
{
	PieceType pieceType = game->Squares[move.From];
	PieceType pt = pieceType & 7;
	U64 fromBit = SquareBitUnchecked(move.From);
	U64 toBit = SquareBitUnchecked(move.To);

	if (pt == KING || move.MoveInfo == EnPassantCapture || move.MoveInfo == CastleShort || move.MoveInfo == CastleLong)
		return FastMoveNeedsFullCheck;

	if (ctx->CheckCount > 1)
		return FastMoveIllegal;

	if (ctx->CheckCount == 1 && (ctx->EvasionMask & toBit) == 0ULL)
		return FastMoveIllegal;

	if ((ctx->Pinned & fromBit) != 0ULL && (ctx->PinLineMasks[move.From] & toBit) == 0ULL)
		return FastMoveIllegal;

	return FastMoveLegal;
}

bool SquareAttacked(int square, Side attackedBy, Game *game)
{
	if (square < 0 || square > 63)
		return false;

	int side01 = attackedBy >> 4;
	const AllPieceBitboards *bitboards = &game->Bitboards;
	U64 pawns = side01 == 0 ? bitboards->Pawns.WhitePawns : bitboards->Pawns.BlackPawns;
	U64 knights = side01 == 0 ? bitboards->Knights.WhiteKnights : bitboards->Knights.BlackKnights;
	U64 bishopsAndQueens = side01 == 0
							  ? bitboards->Bishops.WhiteBishops | bitboards->Queens.WhiteQueens
							  : bitboards->Bishops.BlackBishops | bitboards->Queens.BlackQueens;
	U64 rooksAndQueens = side01 == 0
							 ? bitboards->Rooks.WhiteRooks | bitboards->Queens.WhiteQueens
							 : bitboards->Rooks.BlackRooks | bitboards->Queens.BlackQueens;
	U64 king = side01 == 0 ? bitboards->Kings.WhiteKing : bitboards->Kings.BlackKing;

	if (PatternAttackerFound(pawns, PawnCapturePattern[!side01], square))
		return true;
	if (PatternAttackerFound(knights, 0, square))
		return true;
	if (PatternAttackerFound(king, 1, square))
		return true;
	if (RayAttackerFound(bitboards, bishopsAndQueens, 0, square))
		return true;
	if (RayAttackerFound(bitboards, rooksAndQueens, 1, square))
		return true;

	return false;
}

static void CreateMove(int fromSquare, int toSquare, MoveInfo moveInfo, Game *game, char pieceIdx)
{
	if (game->MovesBufferLength >= MAX_MOVES)
		return;

	Move move;
	move.From = fromSquare;
	move.To = toSquare;
	move.MoveInfo = moveInfo;
	move.PieceIdx = pieceIdx;

	move.Score = GetMoveOrderingScore(move, game);

	// move.Score = GetEval(game, move.Score);

	game->MovesBuffer[game->MovesBufferLength++] = move;
	AssertGame(game);
}

static void AddPromotionMoves(int fromSquare, int toSquare, Game *game, char pieceIdx, bool queenOnly)
{
	CreateMove(fromSquare, toSquare, PromotionQueen, game, pieceIdx);
	if (queenOnly)
		return;

	CreateMove(fromSquare, toSquare, PromotionRook, game, pieceIdx);
	CreateMove(fromSquare, toSquare, PromotionBishop, game, pieceIdx);
	CreateMove(fromSquare, toSquare, PromotionKnight, game, pieceIdx);
}

static void GeneratePawnCaptures(Game *game, int fromSquare, char pieceIdx, bool capturesOnly)
{
	int captPat = PawnCapturePattern[game->Side01];
	int pawnCapPatLength = PieceTypeSquarePatterns[captPat][fromSquare][0];
	Side otherSide = game->Side ^ 24;
	for (int pc = 1; pc <= pawnCapPatLength; pc++)
	{
		int toSquare = PieceTypeSquarePatterns[captPat][fromSquare][pc];
		if (game->Squares[toSquare] & otherSide)
		{
			if (IsPromotionSquare(toSquare))
				AddPromotionMoves(fromSquare, toSquare, game, pieceIdx, capturesOnly);
			else
				CreateMove(fromSquare, toSquare, PlainMove, game, pieceIdx);
		}
		else if (IsEnPassantTargetSquare(game, toSquare))
		{
			CreateMove(fromSquare, toSquare, EnPassantCapture, game, pieceIdx);
		}
	}
}

static void GeneratePawnMoves(Game *game, const Piece *piece, bool capturesOnly)
{
	int fromSquare = piece->SquareIndex;
	char pieceIdx = piece->Index;
	if (capturesOnly)
	{
		int toSquare = fromSquare - Behind[game->Side01];
		if (game->Squares[toSquare] == NOPIECE && IsPromotionSquare(toSquare))
			AddPromotionMoves(fromSquare, toSquare, game, pieceIdx, true);
	}
	else
	{
		int pat = PawnPattern[game->Side01];
		int pawnPatLength = PieceTypeSquarePatterns[pat][fromSquare][0];
		for (int pp = 1; pp <= pawnPatLength; pp++)
		{
			int toSquare = PieceTypeSquarePatterns[pat][fromSquare][pp];
			if (game->Squares[toSquare] != NOPIECE)
				break;

			if (IsPromotionSquare(toSquare))
				AddPromotionMoves(fromSquare, toSquare, game, pieceIdx, false);
			else if ((game->Side == BLACK && toSquare < 24) || (game->Side == WHITE && toSquare > 39))
				CreateMove(fromSquare, toSquare, SoonPromoting, game, pieceIdx);
			else if (pp == 2)
				CreateMove(fromSquare, toSquare, EnPassant, game, pieceIdx);
			else
				CreateMove(fromSquare, toSquare, PlainMove, game, pieceIdx);
		}
	}

	GeneratePawnCaptures(game, fromSquare, pieceIdx, capturesOnly);
}

static void GenerateKnightMoves(Game *game, const Piece *piece, bool capturesOnly)
{
	int fromSquare = piece->SquareIndex;
	char pieceIdx = piece->Index;
	int length = PieceTypeSquarePatterns[0][fromSquare][0];
	Side otherSide = game->Side ^ 24;
	for (int p = 1; p <= length; p++)
	{
		int toSquare = PieceTypeSquarePatterns[0][fromSquare][p];
		PieceType toPiece = game->Squares[toSquare];
		if (capturesOnly)
		{
			if (toPiece & otherSide)
				CreateMove(fromSquare, toSquare, PlainMove, game, pieceIdx);
			continue;
		}

		if (!(toPiece & game->Side))
			CreateMove(fromSquare, toSquare, PlainMove, game, pieceIdx);
	}
}

static void AddCastlingMoves(Game *game, int fromSquare, char pieceIdx)
{
	int castleOffset = CastlesOffset[game->Side01];
	Side otherSide = game->Side ^ 24;
	if (fromSquare != castleOffset + 4)
		return;

	if ((game->Side == WHITE && (game->State & WhiteCanCastleShort)) || (game->Side == BLACK && (game->State & BlackCanCastleShort)))
	{
		if (game->Squares[castleOffset + 7] == (ROOK | game->Side) &&
			game->Squares[castleOffset + 5] == NOPIECE &&
			game->Squares[castleOffset + 6] == NOPIECE)
		{
			if (!SquareAttacked(castleOffset + 5, otherSide, game) && !SquareAttacked(castleOffset + 4, otherSide, game))
				CreateMove(fromSquare, castleOffset + 6, CastleShort, game, pieceIdx);
		}
	}

	if ((game->Side == WHITE && (game->State & WhiteCanCastleLong)) || (game->Side == BLACK && (game->State & BlackCanCastleLong)))
	{
		if (game->Squares[castleOffset] == (ROOK | game->Side) &&
			game->Squares[castleOffset + 1] == NOPIECE &&
			game->Squares[castleOffset + 2] == NOPIECE &&
			game->Squares[castleOffset + 3] == NOPIECE)
		{
			if (!SquareAttacked(castleOffset + 4, otherSide, game) && !SquareAttacked(castleOffset + 3, otherSide, game))
				CreateMove(fromSquare, castleOffset + 2, CastleLong, game, pieceIdx);
		}
	}
}

static void GenerateKingMoves(Game *game, const Piece *piece, bool capturesOnly)
{
	int fromSquare = piece->SquareIndex;
	char pieceIdx = piece->Index;
	int length = PieceTypeSquarePatterns[1][fromSquare][0];
	Side otherSide = game->Side ^ 24;
	for (int p = 1; p <= length; p++)
	{
		int toSquare = PieceTypeSquarePatterns[1][fromSquare][p];
		PieceType toPiece = game->Squares[toSquare];
		if (capturesOnly)
		{
			if (toPiece & otherSide)
				CreateMove(fromSquare, toSquare, KingMove, game, pieceIdx);
			continue;
		}

		if (!(toPiece & game->Side))
			CreateMove(fromSquare, toSquare, KingMove, game, pieceIdx);
	}

	if (!capturesOnly)
		AddCastlingMoves(game, fromSquare, pieceIdx);
}

static void GenerateSliderMoves(Game *game, const Piece *piece, bool capturesOnly)
{
	int fromSquare = piece->SquareIndex;
	char pieceIdx = piece->Index;
	PieceType pt = game->Squares[fromSquare] & 7;
	int pat = pt - 1;
	int raysCount = PieceTypeSquareRaysPatterns[pat][fromSquare][0][0];
	Side otherSide = game->Side ^ 24;
	MoveInfo moveInfo = pt == ROOK ? RookMove : PlainMove;
	for (int r = 1; r <= raysCount; r++)
	{
		int rayLength = PieceTypeSquareRaysPatterns[pat][fromSquare][r][0];
		for (int rr = 1; rr <= rayLength; rr++)
		{
			int toSquare = PieceTypeSquareRaysPatterns[pat][fromSquare][r][rr];
			PieceType toPiece = game->Squares[toSquare];
			if (capturesOnly)
			{
				if (toPiece & otherSide)
					CreateMove(fromSquare, toSquare, moveInfo, game, pieceIdx);
				if (toPiece != NOPIECE)
					break;
				continue;
			}

			if (toPiece != NOPIECE)
			{
				if (!(toPiece & game->Side))
					CreateMove(fromSquare, toSquare, moveInfo, game, pieceIdx);
				break;
			}

			CreateMove(fromSquare, toSquare, moveInfo, game, pieceIdx);
		}
	}
}

static void GeneratePieceMoves(Game *game, const Piece *piece, bool capturesOnly)
{
	PieceType pt = game->Squares[piece->SquareIndex] & 7;
	switch (pt)
	{
	case PAWN:
		GeneratePawnMoves(game, piece, capturesOnly);
		break;
	case KNIGHT:
		GenerateKnightMoves(game, piece, capturesOnly);
		break;
	case KING:
		GenerateKingMoves(game, piece, capturesOnly);
		break;
	default:
		GenerateSliderMoves(game, piece, capturesOnly);
		break;
	}
}

static void CreateMovesForCurrentSide(Game *game, bool capturesOnly)
{
	game->MovesBufferLength = 0;
	Piece *piece = &game->Pieces[game->Side01][0];
	while (piece != NULL)
	{
		GeneratePieceMoves(game, piece, capturesOnly);
		piece = piece->Next;
	}
}

void CreateMoves(Game *game)
{
	CreateMovesForCurrentSide(game, false);
}

void CreateCaptureMoves(Game *game)
{
	CreateMovesForCurrentSide(game, true);
}

void RemoveInvalidMoves(Game *game)
{
	int validMovesCount = 0;
	Move validMoves[MAX_MOVES];
	LegalMoveContext legalCtx;
	BuildLegalMoveContext(game, &legalCtx);

	for (int m = 0; m < game->MovesBufferLength; m++)
	{
		Move move = game->MovesBuffer[m];
		if (move.From > 63 || move.To > 63)
			continue;

		PieceType movingPiece = game->Squares[move.From];
		if (movingPiece == NOPIECE || (movingPiece & 24) != game->Side)
			continue;

		if (move.PieceIdx > 15 || game->Pieces[game->Side01][move.PieceIdx].Off || game->Pieces[game->Side01][move.PieceIdx].SquareIndex != move.From)
		{
			bool found = false;
			for (int p = 0; p < 16; p++)
			{
				Piece *piece = &game->Pieces[game->Side01][p];
				if (!piece->Off && piece->SquareIndex == move.From && piece->Type == movingPiece)
				{
					move.PieceIdx = piece->Index;
					found = true;
					break;
				}
			}
			if (!found)
				continue;
		}

		FastMoveLegality legality = ClassifyMoveLegality(move, game, &legalCtx);
		if (legality == FastMoveIllegal)
			continue;
		if (legality == FastMoveLegal)
		{
			validMoves[validMovesCount++] = move;
			continue;
		}

		Undos undos = DoMove(move, game);
		int kingSquare = game->KingSquares[(game->Side ^ 24) >> 4];
		bool legal = !SquareAttacked(kingSquare, game->Side, game);
		UndoMove(game, move, undos);
		if (legal)
			validMoves[validMovesCount++] = move;
	}
	memcpy(game->MovesBuffer, validMoves, validMovesCount * sizeof(Move));
	game->MovesBufferLength = validMovesCount;
}

int ValidMoves(Move *moves)
{
	CreateMoves(&g_mainGame);
	RemoveInvalidMoves(&g_mainGame);

	if (g_mainGame.MovesBufferLength == 0)
		return 0;

	memcpy(moves, g_mainGame.MovesBuffer, g_mainGame.MovesBufferLength * sizeof(Move));
	return g_mainGame.MovesBufferLength;
}

int ValidMovesOnThread(Game *game, Move *moves)
{
	CreateMoves(game);
	RemoveInvalidMoves(game);

	if (game->MovesBufferLength == 0)
		return 0;

	memcpy(moves, game->MovesBuffer, game->MovesBufferLength * sizeof(Move));
	return game->MovesBufferLength;
}

Move ParseMove(char *sMove, MoveInfo info)
{
	Move move;
	move.From = 255;
	move.To = 255;
	move.MoveInfo = NotAMove;
	move.PieceIdx = 255;
	move.Score = 0;

	if (sMove == NULL || strlen(sMove) < 4)
		return move;

	if (sMove[0] < 'a' || sMove[0] > 'h' || sMove[2] < 'a' || sMove[2] > 'h' ||
		sMove[1] < '1' || sMove[1] > '8' || sMove[3] < '1' || sMove[3] > '8')
		return move;

	int fromFile = sMove[0] - 'a';
	int fromRank = sMove[1] - '1';
	int toFile = sMove[2] - 'a';
	int toRank = sMove[3] - '1';
	move.From = fromRank * 8 + fromFile;
	move.To = toRank * 8 + toFile;
	move.MoveInfo = info;
	if (move.MoveInfo == PlainMove && sMove[4] != '\0')
	{
		switch (sMove[4])
		{
		case 'q':
			move.MoveInfo = PromotionQueen;
			break;
		case 'r':
			move.MoveInfo = PromotionRook;
			break;
		case 'b':
			move.MoveInfo = PromotionBishop;
			break;
		case 'n':
			move.MoveInfo = PromotionKnight;
			break;
		default:
			break;
		}
	}
	return move;
}

PlayerMove MakePlayerMoveOnThread(Game *game, char *sMove)
{
	Move move = ParseMove(sMove, 0);
	PlayerMove playerMove = {0};
	playerMove.Invalid = true;
	bool requestedPromotion = move.MoveInfo >= PromotionQueen && move.MoveInfo <= PromotionKnight;
	if (move.MoveInfo == NotAMove)
		return playerMove;

	Move validMoves[MAX_MOVES];
	int length = ValidMovesOnThread(game, validMoves);
	for (int i = 0; i < length; i++)
	{
		if (validMoves[i].From == move.From &&
			validMoves[i].To == move.To &&
			(!requestedPromotion || validMoves[i].MoveInfo == move.MoveInfo))
		{

			playerMove.Move = validMoves[i];
			playerMove.Invalid = false;
			Undos undos = DoMove(validMoves[i], game);
			playerMove.Undos = undos;
			return playerMove;
		}
	}
	return playerMove;
}

PlayerMove MakePlayerMove(char *sMove)
{
	return MakePlayerMoveOnThread(&g_mainGame, sMove);
}

void UnMakePlayerMove(PlayerMove playerMove)
{
	UndoMove(&g_mainGame, playerMove.Move, playerMove.Undos);
}

void UnMakePlayerMoveOnThread(Game *game, PlayerMove playerMove)
{
	UndoMove(game, playerMove.Move, playerMove.Undos);
}

void MoveToString(Move move, char *sMove)
{
	char fromFile = (move.From & 7) + 'a';
	char fromRank = (move.From >> 3) + '1';
	char toFile = (move.To & 7) + 'a';
	char toRank = (move.To >> 3) + '1';
	sMove[0] = fromFile;
	sMove[1] = fromRank;
	sMove[2] = toFile;
	sMove[3] = toRank;
	switch (move.MoveInfo)
	{
	case PromotionQueen:
		sMove[4] = 'q';
		sMove[5] = '\0';
		break;
	case PromotionRook:
		sMove[4] = 'r';
		sMove[5] = '\0';
		break;
	case PromotionBishop:
		sMove[4] = 'b';
		sMove[5] = '\0';
		break;
	case PromotionKnight:
		sMove[4] = 'n';
		sMove[5] = '\0';
		break;
	default:
		sMove[4] = '\0';
		break;
	}
}

void CoordinatesToString(MoveCoordinates move, char *sMove)
{
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