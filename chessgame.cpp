#include "chessgame.h"

ChessGame::ChessGame() {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            board[r][c].isEmpty = true;
        }
    }
}

void ChessGame::loadSetup(const QVector<Piece>& setup) {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            board[r][c].isEmpty = true;
            board[r][c].type = "";
            board[r][c].color = "";
        }
    }

    for (const Piece& p : setup) {
        if (p.row >= 0 && p.row < 8 && p.col >= 0 && p.col < 8) {
            board[p.row][p.col].type = p.type;
            board[p.row][p.col].color = p.color;
            board[p.row][p.col].isEmpty = false;
        }
    }
}

bool ChessGame::validateSetup(QString& errorMessage) {
    int whiteKings = 0; int blackKings = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (!board[r][c].isEmpty && board[r][c].type == "King") {
                if (board[r][c].color == "White") whiteKings++;
                else if (board[r][c].color == "Black") blackKings++;
            }
        }
    }
    if (whiteKings != 1 || blackKings != 1) {
        errorMessage = "На доске должно быть ровно по одному королю каждого цвета!";
        return false;
    }
    if (isCheckmate("White")) {
        errorMessage = "Белый король под матом! Невозможно начать игру.";
        return false;
    }
    if (isKingInCheck("Black")) {
        errorMessage = "Чёрный король под шахом! Невозможно начать игру.";
        return false;
    }
    if (isStalemate("White")) {
        errorMessage = "Белые находятся в пате! Невозможно начать игру.";
        return false;
    }
    return true;
}

bool ChessGame::isCheckmate(const QString& kingColor) {
    if (!isKingInCheck(kingColor)) {
        return false;
    }

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (!board[r][c].isEmpty && board[r][c].color == kingColor) {
                QVector<ChessMove> moves = getValidMoves(r, c);

                if (!moves.isEmpty()) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool ChessGame::isStalemate(const QString& color) {
    if (isKingInCheck(color)) {
        return false;
    }

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (!board[r][c].isEmpty && board[r][c].color == color) {
                QVector<ChessMove> moves = getValidMoves(r, c);
                if (!moves.isEmpty()) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool ChessGame::isKingInCheck(const QString& color) {
    int kingRow = -1;
    int kingCol = -1;

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (!board[r][c].isEmpty && board[r][c].type == "King" && board[r][c].color == color) {
                kingRow = r;
                kingCol = c;
                break;
            }
        }
        if (kingRow != -1) break;
    }

    if (kingRow == -1) return false;

    QString opponentColor = (color == "White") ? "Black" : "White";
    return isSquareAttacked(kingRow, kingCol, opponentColor);
}

bool ChessGame::isSquareAttacked(int row, int col, const QString& attackingColor) {
    int knightJumps[8][2] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        { 1, -2}, { 1, 2}, { 2, -1}, { 2, 1}
    };
    for (int i = 0; i < 8; ++i) {
        int r = row + knightJumps[i][0];
        int c = col + knightJumps[i][1];
        if (r >= 0 && r < 8 && c >= 0 && c < 8) {
            if (!board[r][c].isEmpty && board[r][c].color == attackingColor && board[r][c].type == "Knight") {
                return true;
            }
        }
    }

    int kingMoves[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        { 0, -1},          { 0, 1},
        { 1, -1}, { 1, 0}, { 1, 1}
    };

    for (int i = 0; i < 8; ++i) {
        int r = row + kingMoves[i][0];
        int c = col + kingMoves[i][1];
        if (r >= 0 && r < 8 && c >= 0 && c < 8) {
            if (!board[r][c].isEmpty && board[r][c].color == attackingColor && board[r][c].type == "King") {
                return true;
            }
        }
    }

    int pawnRowDir = (attackingColor == "White") ? 1 : -1;
    int pawnRow = row + pawnRowDir;

    if (pawnRow >= 0 && pawnRow < 8) {
        if (col - 1 >= 0) {
            if (!board[pawnRow][col - 1].isEmpty &&
                board[pawnRow][col - 1].color == attackingColor &&
                board[pawnRow][col - 1].type == "Pawn") return true;
        }
        if (col + 1 < 8) {
            if (!board[pawnRow][col + 1].isEmpty &&
                board[pawnRow][col + 1].color == attackingColor &&
                board[pawnRow][col + 1].type == "Pawn") return true;
        }
    }

    int straightDirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (int i = 0; i < 4; ++i) {
        int r = row + straightDirs[i][0];
        int c = col + straightDirs[i][1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            if (!board[r][c].isEmpty) {
                if (board[r][c].color == attackingColor &&
                    (board[r][c].type == "Rook" || board[r][c].type == "Queen")) {
                    return true;
                }
                break;
            }
            r += straightDirs[i][0];
            c += straightDirs[i][1];
        }
    }

    int diagDirs[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    for (int i = 0; i < 4; ++i) {
        int r = row + diagDirs[i][0];
        int c = col + diagDirs[i][1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            if (!board[r][c].isEmpty) {
                if (board[r][c].color == attackingColor &&
                    (board[r][c].type == "Bishop" || board[r][c].type == "Queen")) {
                    return true;
                }
                break;
            }
            r += diagDirs[i][0];
            c += diagDirs[i][1];
        }
    }

    return false;
}

QString ChessGame::getPieceColor(int row, int col) {
    if (row < 0 || row > 7 || col < 0 || col > 7 || board[row][col].isEmpty) return "";
    return board[row][col].color;
}

QVector<ChessMove> ChessGame::getValidMoves(int row, int col) {
    QVector<ChessMove> validMoves;
    if (board[row][col].isEmpty) return validMoves;

    QString myColor = board[row][col].color;
    QVector<ChessMove> pseudoMoves = getPseudoLegalMoves(row, col);

    for (const ChessMove& m : pseudoMoves) {
        BoardSquare capturedPiece = board[m.toRow][m.toCol];
        BoardSquare epCapturedPiece;

        if (m.isEnPassant) {
            epCapturedPiece = board[row][m.toCol];
            board[row][m.toCol].isEmpty = true;
        }

        board[m.toRow][m.toCol] = board[row][col];
        board[row][col].isEmpty = true;

        if (!isKingInCheck(myColor)) {
            validMoves.append(m);
        }

        board[row][col] = board[m.toRow][m.toCol];
        board[m.toRow][m.toCol] = capturedPiece;

        if (m.isEnPassant) {
            board[row][m.toCol] = epCapturedPiece;
        }
    }
    return validMoves;
}

QVector<ChessMove> ChessGame::getPseudoLegalMoves(int row, int col) {
    QVector<ChessMove> moves;
    if (board[row][col].isEmpty) return moves;

    QString type = board[row][col].type;
    QString color = board[row][col].color;

    if (type == "Rook") return getRookMoves(row, col, color);
    if (type == "Bishop") return getBishopMoves(row, col, color);
    if (type == "Queen") return getQueenMoves(row, col, color);
    if (type == "Knight") return getKnightMoves(row, col, color);
    if (type == "King") return getKingMoves(row, col, color);
    if (type == "Pawn") return getPawnMoves(row, col, color);

    return moves;
}

QVector<ChessMove> ChessGame::getRookMoves(int row, int col, const QString& color) {
    QVector<ChessMove> moves;

    int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    for (int i = 0; i < 4; ++i) {
        int r = row + directions[i][0];
        int c = col + directions[i][1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            if (board[r][c].isEmpty) {
                moves.append(ChessMove(r, c, false));
            } else {
                if (board[r][c].color != color) {
                    moves.append(ChessMove(r, c, true));
                }
                break;
            }
            r += directions[i][0];
            c += directions[i][1];
        }
    }

    return moves;
}

QVector<ChessMove> ChessGame::getBishopMoves(int row, int col, const QString& color) {
    QVector<ChessMove> moves;

    int directions[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

    for (int i = 0; i < 4; ++i) {
        int r = row + directions[i][0];
        int c = col + directions[i][1];

        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            if (board[r][c].isEmpty) {
                moves.append(ChessMove(r, c, false));
            } else {
                if (board[r][c].color != color) {
                    moves.append(ChessMove(r, c, true));
                }
                break;
            }
            r += directions[i][0];
            c += directions[i][1];
        }
    }
    return moves;
}

QVector<ChessMove> ChessGame::getQueenMoves(int row, int col, const QString& color) {
    QVector<ChessMove> moves = getRookMoves(row, col, color);
    moves.append(getBishopMoves(row, col, color));
    return moves;
}

QVector<ChessMove> ChessGame::getKnightMoves(int row, int col, const QString& color) {
    QVector<ChessMove> moves;

    int jumps[8][2] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        {1, -2}, {1, 2}, {2, -1}, {2, 1}
    };

    for (int i = 0; i < 8; ++i) {
        int r = row + jumps[i][0];
        int c = col + jumps[i][1];

        if (r >= 0 && r < 8 && c >= 0 && c < 8) {
            if (board[r][c].isEmpty) {
                moves.append(ChessMove(r, c, false));
            } else if (board[r][c].color != color) {
                moves.append(ChessMove(r, c, true));
            }
        }
    }
    return moves;
}

QVector<ChessMove> ChessGame::getKingMoves(int row, int col, const QString& color) {
    QVector<ChessMove> moves;

    int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        { 0, -1},          { 0, 1},
        { 1, -1}, { 1, 0}, { 1, 1}
    };

    for (int i = 0; i < 8; ++i) {
        int r = row + directions[i][0];
        int c = col + directions[i][1];

        if (r >= 0 && r < 8 && c >= 0 && c < 8) {
            if (board[r][c].isEmpty) {
                moves.append(ChessMove(r, c, false));
            } else if (board[r][c].color != color) {
                moves.append(ChessMove(r, c, true));
            }
        }
    }

    if (globalCastlingAllowed) {
        if (color == "White" && !wKingMoved && row == 7 && col == 4 && !isKingInCheck("White")) {
            if (!wRookRMoved && board[7][5].isEmpty && board[7][6].isEmpty &&
                !isSquareAttacked(7, 5, "Black") && !isSquareAttacked(7, 6, "Black")) {
                moves.append(ChessMove(7, 6, false, true, false));
            }
            if (!wRookLMoved && board[7][1].isEmpty && board[7][2].isEmpty && board[7][3].isEmpty &&
                !isSquareAttacked(7, 2, "Black") && !isSquareAttacked(7, 3, "Black")) {
                moves.append(ChessMove(7, 2, false, true, false));
            }
        } else if (color == "Black" && !bKingMoved && row == 0 && col == 4 && !isKingInCheck("Black")) {
            if (!bRookRMoved && board[0][5].isEmpty && board[0][6].isEmpty &&
                !isSquareAttacked(0, 5, "White") && !isSquareAttacked(0, 6, "White")) {
                moves.append(ChessMove(0, 6, false, true, false));
            }
            if (!bRookLMoved && board[0][1].isEmpty && board[0][2].isEmpty && board[0][3].isEmpty &&
                !isSquareAttacked(0, 2, "White") && !isSquareAttacked(0, 3, "White")) {
                moves.append(ChessMove(0, 2, false, true, false));
            }
        }
    }

    return moves;
}

QVector<ChessMove> ChessGame::getPawnMoves(int row, int col, const QString& color) {
    QVector<ChessMove> moves;

    int dir = (color == "White") ? -1 : 1;
    int startRow = (color == "White") ? 6 : 1;

    int rForward = row + dir;
    if (rForward >= 0 && rForward < 8) {
        if (board[rForward][col].isEmpty) {
            moves.append(ChessMove(rForward, col, false));

            int rDoubleForward = row + dir * 2;
            if (row == startRow && board[rDoubleForward][col].isEmpty) {
                moves.append(ChessMove(rDoubleForward, col, false));
            }
        }
    }

    int captureCols[2] = {col - 1, col + 1};
    for (int i = 0; i < 2; ++i) {
        int c = captureCols[i];
        if (rForward >= 0 && rForward < 8 && c >= 0 && c < 8) {
            if (!board[rForward][c].isEmpty && board[rForward][c].color != color) {
                moves.append(ChessMove(rForward, c, true));
            }
            if (rForward == epRow && c == epCol) {
                moves.append(ChessMove(rForward, c, true, false, true));
            }
        }
    }

    return moves;
}

QString ChessGame::executeMove(int fromRow, int fromCol, int toRow, int toCol, const QString& promotionType) {
    QString capturedPieceType = "";
    QString movingPiece = board[fromRow][fromCol].type;
    QString movingColor = board[fromRow][fromCol].color;

    bool isCastling = (movingPiece == "King" && std::abs(toCol - fromCol) == 2);
    bool isEP = (movingPiece == "Pawn" && fromCol != toCol && board[toRow][toCol].isEmpty);

    if (isEP) {
        capturedPieceType = "Pawn";
        board[fromRow][toCol].isEmpty = true;
        board[fromRow][toCol].type = "";
        board[fromRow][toCol].color = "";
    } else if (!board[toRow][toCol].isEmpty) {
        capturedPieceType = board[toRow][toCol].type;
    }

    if (isEP || !capturedPieceType.isEmpty() || movingPiece == "Pawn") {
        halfMoveClock = 0;
    } else {
        halfMoveClock++;
    }

    if (movingPiece == "Pawn" && (toRow == 0 || toRow == 7) && !promotionType.isEmpty()) {
        board[toRow][toCol].type = promotionType;
        board[toRow][toCol].color = movingColor;
        board[toRow][toCol].isEmpty = false;
    } else {
        board[toRow][toCol] = board[fromRow][fromCol];
    }

    board[fromRow][fromCol].isEmpty = true;
    board[fromRow][fromCol].type = "";
    board[fromRow][fromCol].color = "";

    if (isCastling) {
        if (toCol == 6) {
            board[toRow][5] = board[toRow][7];
            board[toRow][7].isEmpty = true;
            board[toRow][7].type = ""; board[toRow][7].color = "";
        } else if (toCol == 2) {
            board[toRow][3] = board[toRow][0];
            board[toRow][0].isEmpty = true;
            board[toRow][0].type = ""; board[toRow][0].color = "";
        }
    }

    if (movingPiece == "King") {
        if (movingColor == "White") wKingMoved = true;
        else bKingMoved = true;
    } else if (movingPiece == "Rook") {
        if (movingColor == "White") {
            if (fromRow == 7 && fromCol == 0) wRookLMoved = true;
            if (fromRow == 7 && fromCol == 7) wRookRMoved = true;
        } else {
            if (fromRow == 0 && fromCol == 0) bRookLMoved = true;
            if (fromRow == 0 && fromCol == 7) bRookRMoved = true;
        }
    }

    if (movingPiece == "Pawn" && std::abs(toRow - fromRow) == 2) {
        epRow = fromRow + ((movingColor == "White") ? -1 : 1);
        epCol = fromCol;
    } else {
        epRow = -1;
        epCol = -1;
    }

    return capturedPieceType;
}

void ChessGame::setGameMode(bool isStandardSetup) {
    globalCastlingAllowed = isStandardSetup;
    wKingMoved = false; wRookLMoved = false; wRookRMoved = false;
    bKingMoved = false; bRookLMoved = false; bRookRMoved = false;
    epRow = -1; epCol = -1;
}

QVector<Piece> ChessGame::getActivePieces() const {
    QVector<Piece> pieces;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (!board[r][c].isEmpty) {
                pieces.append({board[r][c].type, board[r][c].color, r, c});
            }
        }
    }
    return pieces;
}

void ChessGame::resetDrawCounters() {
    halfMoveClock = 0;
    positionHistory.clear();
}

QString ChessGame::generatePositionHash(const QString& activeColor) const {
    QString hash;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (board[r][c].isEmpty) hash += ".";
            else {
                hash += board[r][c].color[0];
                hash += board[r][c].type[0];
            }
        }
    }
    hash += wKingMoved ? "0" : "1";
    hash += wRookLMoved ? "0" : "1";
    hash += wRookRMoved ? "0" : "1";
    hash += bKingMoved ? "0" : "1";
    hash += bRookLMoved ? "0" : "1";
    hash += bRookRMoved ? "0" : "1";
    hash += QString::number(epCol);
    hash += activeColor[0];

    return hash;
}

void ChessGame::recordPosition(const QString& activeColor) {
    QString hash = generatePositionHash(activeColor);
    positionHistory[hash]++;
}

bool ChessGame::isFiftyMoveRule() const {
    return halfMoveClock >= 100;
}

bool ChessGame::isThreefoldRepetition() const {
    for (int count : positionHistory.values()) {
        if (count >= 3) return true;
    }
    return false;
}

int ChessGame::getMaterialScore(const QString& color) const {
    int score = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (!board[r][c].isEmpty && board[r][c].color == color) {
                QString type = board[r][c].type;
                if (type == "Pawn") score += 1;
                else if (type == "Knight" || type == "Bishop") score += 3;
                else if (type == "Rook") score += 5;
                else if (type == "Queen") score += 9;
            }
        }
    }
    return score;
}

bool ChessGame::isInsufficientMaterial() const {
    int wPawns = 0, wRooks = 0, wKnights = 0, wBishops = 0, wQueens = 0;
    int bPawns = 0, bRooks = 0, bKnights = 0, bBishops = 0, bQueens = 0;

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (!board[r][c].isEmpty) {
                QString t = board[r][c].type;
                if (board[r][c].color == "White") {
                    if (t == "Pawn") wPawns++;
                    else if (t == "Rook") wRooks++;
                    else if (t == "Knight") wKnights++;
                    else if (t == "Bishop") wBishops++;
                    else if (t == "Queen") wQueens++;
                } else {
                    if (t == "Pawn") bPawns++;
                    else if (t == "Rook") bRooks++;
                    else if (t == "Knight") bKnights++;
                    else if (t == "Bishop") bBishops++;
                    else if (t == "Queen") bQueens++;
                }
            }
        }
    }

    if (wPawns > 0 || bPawns > 0 || wRooks > 0 || bRooks > 0 || wQueens > 0 || bQueens > 0) {
        return false;
    }

    if (wKnights == 0 && wBishops == 0 && bKnights == 0 && bBishops == 0) {
        return true;
    }

    if (wKnights == 0 && wBishops == 0) {
        if ((bBishops == 1 && bKnights == 0) || (bKnights == 1 && bBishops == 0) || (bKnights == 2 && bBishops == 0)) {
            return true;
        }
    }

    if (bKnights == 0 && bBishops == 0) {
        if ((wBishops == 1 && wKnights == 0) || (wKnights == 1 && wBishops == 0) || (wKnights == 2 && wBishops == 0)) {
            return true;
        }
    }

    return false;
}
