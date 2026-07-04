#ifndef CHESSGAME_H
#define CHESSGAME_H

#include <QVector>
#include <QString>
#include <QMap>
#include "chessdata.h"

struct BoardSquare {
    QString type;
    QString color;
    bool isEmpty = true;
};

struct ChessMove {
    int toRow;
    int toCol;
    bool isCapture;
    bool isCastling;
    bool isEnPassant;

    ChessMove(int r=0, int c=0, bool cap=false, bool castl=false, bool ep=false)
        : toRow(r), toCol(c), isCapture(cap), isCastling(castl), isEnPassant(ep) {}
};

class ChessGame {
public:
    ChessGame();

    void loadSetup(const QVector<Piece>& setup);
    bool validateSetup(QString& errorMessage);
    bool isCheckmate(const QString& kingColor);
    bool isStalemate(const QString& color);
    bool isInsufficientMaterial() const;
    QString getPieceColor(int row, int col);
    QVector<ChessMove> getValidMoves(int row, int col);
    QString executeMove(int fromRow, int fromCol, int toRow, int toCol, const QString& promotionType = "");
    void setGameMode(bool isStandardSetup);
    QVector<Piece> getActivePieces() const;
    bool isFiftyMoveRule() const;
    bool isThreefoldRepetition() const;
    void recordPosition(const QString& activeColor);
    void resetDrawCounters();
    int getMaterialScore(const QString& color) const;

private:
    BoardSquare board[8][8];
    int halfMoveClock = 0;

    QMap<QString, int> positionHistory;
    QString generatePositionHash(const QString& activeColor) const;

    bool isKingInCheck(const QString& color);
    bool isSquareAttacked(int row, int col, const QString& attackingColor);
    QVector<ChessMove> getPseudoLegalMoves(int row, int col);
    QVector<ChessMove> getRookMoves(int row, int col, const QString& color);
    QVector<ChessMove> getBishopMoves(int row, int col, const QString& color);
    QVector<ChessMove> getQueenMoves(int row, int col, const QString& color);
    QVector<ChessMove> getKnightMoves(int row, int col, const QString& color);
    QVector<ChessMove> getKingMoves(int row, int col, const QString& color);
    QVector<ChessMove> getPawnMoves(int row, int col, const QString& color);

    bool globalCastlingAllowed = false;
    bool wKingMoved = false, wRookLMoved = false, wRookRMoved = false;
    bool bKingMoved = false, bRookLMoved = false, bRookRMoved = false;
    int epRow = -1, epCol = -1;
};

#endif // CHESSGAME_H
