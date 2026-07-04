#ifndef CHESSDATA_H
#define CHESSDATA_H

#include <QString>

struct Piece {
    QString type;
    QString color;
    int row;
    int col;
};

#endif // CHESSDATA_H
