#include "see.h"

#include "moves.h"

static short GetPromotionMaterialGain(MoveInfo moveInfo)
{
    switch (moveInfo)
    {
    case PromotionQueen:
        return MATERIAL_Q - MATERIAL_P;
    case PromotionRook:
        return MATERIAL_R - MATERIAL_P;
    case PromotionBishop:
        return MATERIAL_B - MATERIAL_P;
    case PromotionKnight:
        return MATERIAL_N - MATERIAL_P;
    default:
        return 0;
    }
}

static short GetMoveMaterialSwing(Move move, PieceType capturedPiece)
{
    short captureGain = move.MoveInfo == EnPassantCapture ? MATERIAL_P : GetPieceMaterialValue(capturedPiece);
    return (short)(captureGain + GetPromotionMaterialGain((MoveInfo)move.MoveInfo));
}

static short StaticExchangeEvaluationRecursive(int targetSquare, Game *game)
{
    CreateCaptureMoves(game);
    RemoveInvalidMoves(game);

    short bestGain = 0;
    for (int i = 0; i < game->MovesBufferLength; i++)
    {
        Move recapture = game->MovesBuffer[i];
        if (recapture.To != targetSquare)
            continue;

        PieceType capturedPiece = game->Squares[targetSquare];
        if (capturedPiece == NOPIECE)
            continue;

        short captureGain = GetMoveMaterialSwing(recapture, capturedPiece);
        Undos undos = DoMove(recapture, game);
        short exchangeGain = (short)(captureGain - StaticExchangeEvaluationRecursive(targetSquare, game));
        UndoMove(game, recapture, undos);

        if (exchangeGain > bestGain)
            bestGain = exchangeGain;
    }

    return bestGain;
}

short StaticExchangeEvaluation(Move move, Game *game)
{
    PieceType capturedPiece = game->Squares[move.To];
    if (capturedPiece == NOPIECE && move.MoveInfo != EnPassantCapture)
        return 0;

    short immediateGain = GetMoveMaterialSwing(move, capturedPiece);
    Undos undos = DoMove(move, game);
    short replyGain = StaticExchangeEvaluationRecursive(move.To, game);
    UndoMove(game, move, undos);

    return (short)(immediateGain - replyGain);
}