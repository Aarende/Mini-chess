#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QTimer>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include "chessdata.h"
#include "chessgame.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStandardGameClicked();
    void onCustomGameClicked();
    void onContinueGameClicked();
    void onPauseClicked();
    void onStartGameClicked();
    void onTimerTick();
    void onSaveSetupTriggered();
    void onLoadSetupTriggered();
    void onEndGameClicked();
    void onSaveGameTriggered();
    void onLoadGameTriggered();
    void handleCellClick(int row, int col, bool isLeftClick);

private:
    Ui::MainWindow *ui;
    QVector<Piece> currentSetup;
    bool isGameInitialized = false;
    bool isGameActive = false;
    bool isPaused = false;
    QString activePlayer = "White";
    bool isPostGame = false;
    QVector<Piece> initialSetup;
    int whiteTimeLeft = 0;
    int blackTimeLeft = 0;
    QTimer *gameTimer;
    QFrame *pauseOverlay;
    QFrame *menuOverlay;
    ChessGame *gameEngine;
    int whiteErrors = 0;
    int blackErrors = 0;
    int whiteScore = 0;
    int blackScore = 0;
    int selectedRow = -1;
    int selectedCol = -1;

    void initStandardSetup();
    void removePieceAt(int row, int col);
    void addPieceAt(int row, int col, QString type, QString color);
    void updateTimerLabels();
    void changeTurn();
    void incrementError();
    void performMove(int fromRow, int fromCol, int toRow, int toCol);
    int getPieceValue(const QString& type);
    void updateScoreUI();
    bool checkIfStandardSetup();
    void showPostGameDialog(const QString& title, const QString& message);
    void restoreSetupMode();
    void replayMove(const QString& moveStr);

protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif // MAINWINDOW_H