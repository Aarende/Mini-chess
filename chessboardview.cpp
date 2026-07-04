#include "chessboardview.h"
#include <QMouseEvent>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QDebug>
#include <QtSvgWidgets/QGraphicsSvgItem>
#include <QtSvg/QSvgRenderer>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>

ChessBoardView::ChessBoardView(QWidget *parent) : QGraphicsView(parent) {
    scene = new QGraphicsScene(this);
    setScene(scene);
    scene->setBackgroundBrush(Qt::black);
    setRenderHint(QPainter::Antialiasing);
    setMouseTracking(true);

    errorAnim = new QVariantAnimation(this);
    errorAnim->setDuration(400);
    errorAnim->setStartValue(QColor(Qt::black));
    errorAnim->setKeyValueAt(0.5, QColor(255, 165, 0));
    errorAnim->setEndValue(QColor(Qt::black));
    connect(errorAnim, &QVariantAnimation::valueChanged, [=](const QVariant &val){
        scene->setBackgroundBrush(val.value<QColor>());
    });
}

void ChessBoardView::drawBoard() {
    clearMoveHints();

    scene->clear();
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            QGraphicsRectItem *tile = scene->addRect(
                col * tileSize + margin,
                row * tileSize + margin,
                tileSize, tileSize
                );
            tile->setBrush((row + col) % 2 == 0 ? colorLight : colorDark);
            tile->setPen(Qt::NoPen);
        }
    }

    QFont font("Segoe UI", 11, QFont::Bold);
    QColor textColor(Qt::white);

    for (int col = 0; col < 8; ++col) {
        QString letter = QString(QChar('A' + col));
        QGraphicsTextItem *textItem = scene->addText(letter, font);
        textItem->setDefaultTextColor(textColor);

        qreal textWidth = textItem->boundingRect().width();
        qreal x = col * tileSize + margin + (tileSize - textWidth) / 2;
        qreal y = margin - 25;

        textItem->setPos(x, y);
        textItem->setZValue(1);
    }

    for (int row = 0; row < 8; ++row) {
        QString number = QString::number(8 - row);
        QGraphicsTextItem *textItem = scene->addText(number, font);
        textItem->setDefaultTextColor(textColor);

        qreal textWidth = textItem->boundingRect().width();
        qreal textHeight = textItem->boundingRect().height();

        qreal x = margin - textWidth - 10;

        qreal y = row * tileSize + margin + (tileSize - textHeight) / 2;

        textItem->setPos(x, y);
        textItem->setZValue(1);
    }

    hoverRect = scene->addRect(0, 0, tileSize, tileSize);
    hoverRect->setBrush(QColor(128, 128, 128, 100));
    hoverRect->setPen(Qt::NoPen);
    hoverRect->setZValue(1);
    hoverRect->setVisible(false);
}

void ChessBoardView::updatePieces(const QVector<Piece> &pieces) {
    drawBoard();

    for (const Piece &p : pieces) {
        QString path = QString(":/assets/%1_%2.svg").arg(p.color.toLower(), p.type.toLower());
        QGraphicsSvgItem *svgItem = new QGraphicsSvgItem(path);

        if (!svgItem->renderer()->isValid()) {
            qDebug() << "Не удалось загрузить SVG:" << path;
            delete svgItem;
            continue;
        }

        QRectF bounds = svgItem->boundingRect();
        qreal scaleX = qreal(tileSize) / bounds.width();
        qreal scaleY = qreal(tileSize) / bounds.height();

        svgItem->setScale(qreal(qMin(scaleX, scaleY)));
        svgItem->setPos(p.col * tileSize + margin, p.row * tileSize + margin);

        svgItem->setZValue(2);

        scene->addItem(svgItem);
    }
}

void ChessBoardView::mousePressEvent(QMouseEvent *event) {
    QPointF scenePos = mapToScene(event->pos());

    int x = static_cast<int>(scenePos.x()) - margin;
    int y = static_cast<int>(scenePos.y()) - margin;

    if (x >= 0 && y >= 0) {
        int col = x / tileSize;
        int row = y / tileSize;

        if (col >= 0 && col < 8 && row >= 0 && row < 8) {
            if (event->button() == Qt::LeftButton) {
                emit cellClicked(row, col, true);
            } else if (event->button() == Qt::RightButton) {
                emit cellClicked(row, col, false);
            }
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void ChessBoardView::mouseMoveEvent(QMouseEvent *event) {
    if (!hoverRect) return;

    QPointF scenePos = mapToScene(event->pos());

    int x = static_cast<int>(scenePos.x()) - margin;
    int y = static_cast<int>(scenePos.y()) - margin;

    if (x >= 0 && y >= 0) {
        int col = x / tileSize;
        int row = y / tileSize;

        if (col >= 0 && col < 8 && row >= 0 && row < 8) {
            hoverRect->setPos(col * tileSize + margin, row * tileSize + margin);
            hoverRect->setVisible(true);
        } else {
            hoverRect->setVisible(false);
        }
    } else {
        hoverRect->setVisible(false);
    }

    QGraphicsView::mouseMoveEvent(event);
}

void ChessBoardView::leaveEvent(QEvent *event) {
    if (hoverRect) {
        hoverRect->setVisible(false);
    }
    QGraphicsView::leaveEvent(event);
}

void ChessBoardView::showMoveHints(const QVector<ChessMove>& moves) {
    clearMoveHints();

    for (const ChessMove& move : moves) {
        int x = move.toCol * tileSize + margin;
        int y = move.toRow * tileSize + margin;

        QGraphicsEllipseItem *dot = new QGraphicsEllipseItem();

        if (move.isCastling) {
            dot->setRect(x + tileSize * 0.3, y + tileSize * 0.3, tileSize * 0.4, tileSize * 0.4);
            dot->setBrush(QBrush(QColor(135, 206, 235, 180)));
        } else if (move.isCapture) {
            dot->setRect(x + tileSize * 0.1, y + tileSize * 0.1, tileSize * 0.8, tileSize * 0.8);
            dot->setBrush(QBrush(QColor(128, 128, 128, 150)));
        } else {
            dot->setRect(x + tileSize * 0.3, y + tileSize * 0.3, tileSize * 0.4, tileSize * 0.4);
            dot->setBrush(QBrush(QColor(255, 255, 255, 180)));
        }

        dot->setPen(Qt::NoPen);
        dot->setZValue(3);
        scene->addItem(dot);
        moveHints.append(dot);
    }
}

void ChessBoardView::clearMoveHints() {
    for (QGraphicsItem* item : moveHints) {
        scene->removeItem(item);
        delete item;
    }
    moveHints.clear();
}

void ChessBoardView::triggerErrorAnimation() {
    if (errorAnim->state() == QAbstractAnimation::Running) {
        errorAnim->stop();
    }
    errorAnim->start();
}
