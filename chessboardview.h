#ifndef CHESSBOARDVIEW_H
#define CHESSBOARDVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QVariantAnimation>
#include "chessdata.h"
#include "chessgame.h"

class ChessBoardView : public QGraphicsView {
    Q_OBJECT
public:
    explicit ChessBoardView(QWidget *parent = nullptr);
    void drawBoard();
    void updatePieces(const QVector<Piece> &pieces);
    void showMoveHints(const QVector<ChessMove>& moves);
    void clearMoveHints();
    void triggerErrorAnimation();

signals:
    void cellClicked(int row, int col, bool isLeftClick);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QGraphicsScene *scene;
    const QColor colorLight = QColor(0xEEEED2);
    const QColor colorDark = QColor(0x769656);
    const int tileSize = 80;
    const int margin = 30;
    QGraphicsRectItem *hoverRect = nullptr;
    QVector<QGraphicsItem*> moveHints;
    QVariantAnimation *errorAnim;
};

#endif // CHESSBOARDVIEW_H
