#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QBuffer>
#include <QDir>
#include <QtSvg/QSvgRenderer>
#include <QShortcut>
#include <QKeySequence>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->boardView->drawBoard();

    gameEngine = new ChessGame();

    gameTimer = new QTimer(this);

    connect(gameTimer, &QTimer::timeout, this, &MainWindow::onTimerTick);
    connect(ui->btnStandard, &QPushButton::clicked, this, &MainWindow::onStandardGameClicked);
    connect(ui->btnCustom, &QPushButton::clicked, this, &MainWindow::onCustomGameClicked);
    connect(ui->btnContinue, &QPushButton::clicked, this, &MainWindow::onContinueGameClicked);
    connect(ui->btnPause, &QPushButton::clicked, this, &MainWindow::onPauseClicked);
    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::onStartGameClicked);
    connect(ui->actionSaveSetup, &QAction::triggered, this, &MainWindow::onSaveSetupTriggered);
    connect(ui->actionLoadSetup, &QAction::triggered, this, &MainWindow::onLoadSetupTriggered);
    connect(ui->actionSaveGame, &QAction::triggered, this, &MainWindow::onSaveGameTriggered);
    connect(ui->actionLoadGame, &QAction::triggered, this, &MainWindow::onLoadGameTriggered);
    connect(ui->boardView, &ChessBoardView::cellClicked, this, &MainWindow::handleCellClick);
    connect(ui->btnEndGame, &QPushButton::clicked, this, &MainWindow::onEndGameClicked);

    QShortcut *fullscreenShortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(fullscreenShortcut, &QShortcut::activated, this, [this]() {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    });

    ui->btnEndGame->setEnabled(false);

    ui->btnPause->setEnabled(false);
    ui->btnContinue->setEnabled(false);

    pauseOverlay = new QFrame(ui->boardView);
    pauseOverlay->setStyleSheet("background-color: #262522;");
    pauseOverlay->setVisible(false);

    QVBoxLayout *overlayLayout = new QVBoxLayout(pauseOverlay);
    QLabel *pauseLabel = new QLabel("Игра на паузе", pauseOverlay);
    pauseLabel->setAlignment(Qt::AlignCenter);
    pauseLabel->setStyleSheet("color: #FFFFFF; font-size: 28px; font-weight: bold; font-family: 'Segoe UI', Arial;");
    overlayLayout->addWidget(pauseLabel);

    menuOverlay = new QFrame(ui->boardView);
    menuOverlay->setStyleSheet("background-color: #262522;");
    menuOverlay->resize(700, 700);

    QVBoxLayout *menuLayout = new QVBoxLayout(menuOverlay);
    QLabel *menuLabel = new QLabel("Выберите тип игры\nили загрузите сохранения из файла", menuOverlay);
    menuLabel->setAlignment(Qt::AlignCenter);
    menuLabel->setStyleSheet("color: #FFFFFF; font-size: 22px; font-weight: bold; font-family: 'Segoe UI', Arial;");
    menuLayout->addWidget(menuLabel);

    menuOverlay->setVisible(true);
}

MainWindow::~MainWindow() {
    delete ui;
    delete gameEngine;
}

void MainWindow::initStandardSetup() {
    if (isGameActive) return;
    currentSetup.clear();
    QStringList order = {"Rook", "Knight", "Bishop", "Queen", "King", "Bishop", "Knight", "Rook"};
    for (int col = 0; col < 8; ++col) {
        currentSetup.append({order[col], "Black", 0, col});
        currentSetup.append({"Pawn", "Black", 1, col});
        currentSetup.append({"Pawn", "White", 6, col});
        currentSetup.append({order[col], "White", 7, col});
    }
    isGameInitialized = true;
    ui->boardView->updatePieces(currentSetup);
}

void MainWindow::onStandardGameClicked() {
    if (isGameActive) return;
    menuOverlay->setVisible(false);
    initStandardSetup();
}

void MainWindow::onCustomGameClicked() {
    if (isGameActive) return;
    menuOverlay->setVisible(false);
    currentSetup.clear();
    isGameInitialized = true;
    ui->boardView->updatePieces(currentSetup);
    ui->lblCapturedWhite->setText("Съедено (Белые):");
    ui->lblCapturedBlack->setText("Съедено (Чёрные):");
}

void MainWindow::onStartGameClicked() {
    if (!isGameInitialized || currentSetup.isEmpty() || isGameActive) return;

    gameEngine->loadSetup(currentSetup);

    gameEngine->resetDrawCounters();
    gameEngine->recordPosition("White");

    QString errorMsg;
    if (!gameEngine->validateSetup(errorMsg)) {
        QMessageBox::critical(this, "Ошибка старта", errorMsg);
        return;
    }

    bool isStandard = checkIfStandardSetup();
    if (!checkIfStandardSetup()) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Предупреждение");
        msgBox.setText("Вы используете нестандартную расстановку.\nВ этой партии рокировка будет недоступна. Продолжить?");
        msgBox.setIcon(QMessageBox::Question);

        msgBox.addButton("Да", QMessageBox::YesRole);
        QPushButton *noButton = msgBox.addButton("Нет", QMessageBox::NoRole);

        msgBox.exec();

        if (msgBox.clickedButton() == noButton) {
            return;
        }
    }
    gameEngine->setGameMode(isStandard);

    whiteErrors = 0;
    blackErrors = 0;
    whiteScore = 0;
    blackScore = 0;
    updateScoreUI();
    ui->listMoves->clear();
    ui->lblErrorsWhite->setText("Ошибки Белые: 0");
    ui->lblErrorsBlack->setText("Ошибки Чёрные: 0");

    initialSetup = currentSetup;
    isPostGame = false;

    ui->btnEndGame->setEnabled(true);
    ui->btnEndGame->setText("Сдаться");

    int totalSeconds = ui->spinTime->value() * 60;
    whiteTimeLeft = totalSeconds;
    blackTimeLeft = totalSeconds;
    updateTimerLabels();

    isGameActive = true;
    isPaused = false;
    activePlayer = "White";

    ui->btnStart->setEnabled(false);
    ui->btnStandard->setEnabled(false);
    ui->btnCustom->setEnabled(false);
    ui->spinTime->setEnabled(false);

    ui->btnPause->setEnabled(true);
    ui->btnContinue->setEnabled(false);

    gameTimer->start(1000);
    qDebug() << "Игра началась!";
}

void MainWindow::onPauseClicked() {
    if (!isGameActive || isPaused) return;

    isPaused = true;
    gameTimer->stop();

    pauseOverlay->resize(ui->boardView->size());
    pauseOverlay->setVisible(true);

    ui->btnPause->setEnabled(false);
    ui->btnContinue->setEnabled(true);

    qDebug() << "Игра поставлена на паузу. Фигуры скрыты.";
}

void MainWindow::onContinueGameClicked() {
    if (!isGameActive || !isPaused) return;

    isPaused = false;

    pauseOverlay->setVisible(false);

    ui->btnPause->setEnabled(true);
    ui->btnContinue->setEnabled(false);

    gameTimer->start(1000);
    qDebug() << "Игра возобновлена.";
}

void MainWindow::onTimerTick() {
    if (!isGameActive || isPaused) return;

    if (activePlayer == "White") {
        if (whiteTimeLeft > 0) whiteTimeLeft--;
        else showPostGameDialog("Время истекло", "Время Белых вышло! Чёрные победили.");
    } else {
        if (blackTimeLeft > 0) blackTimeLeft--;
        else showPostGameDialog("Время истекло", "Время Чёрных вышло! Белые победили.");
    }
    updateTimerLabels();
}

void MainWindow::updateTimerLabels() {
    auto formatTime = [](int totalSecs) -> QString {
        int mins = totalSecs / 60;
        int secs = totalSecs % 60;
        return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
    };
    ui->lblTimerWhite->setText("Время Белые: " + formatTime(whiteTimeLeft));
    ui->lblTimerBlack->setText("Время Чёрные: " + formatTime(blackTimeLeft));
}

void MainWindow::handleCellClick(int row, int col, bool isLeftClick) {
    if (isGameActive) {
        if (isPaused || !isLeftClick) return;

        QString clickedColor = gameEngine->getPieceColor(row, col);

        if (selectedRow != -1 && selectedCol != -1) {

            if (row == selectedRow && col == selectedCol) {
                selectedRow = -1; selectedCol = -1;
                ui->boardView->clearMoveHints();
                return;
            }

            if (clickedColor == activePlayer) {
                selectedRow = row; selectedCol = col;
                ui->boardView->showMoveHints(gameEngine->getValidMoves(row, col));
                return;
            }

            QVector<ChessMove> moves = gameEngine->getValidMoves(selectedRow, selectedCol);
            bool isValidMove = false;
            for (const ChessMove& m : moves) {
                if (m.toRow == row && m.toCol == col) {
                    isValidMove = true;
                    break;
                }
            }

            if (isValidMove) {
                performMove(selectedRow, selectedCol, row, col);
            } else {
                incrementError();
            }
        }
        else {
            if (clickedColor == activePlayer) {
                selectedRow = row; selectedCol = col;
                ui->boardView->showMoveHints(gameEngine->getValidMoves(row, col));
            } else {
                incrementError();
            }
        }
        return;
    }

    if (isPostGame) return;

    if (!isGameInitialized) return;

    if (!isLeftClick) {
        removePieceAt(row, col);
        ui->boardView->updatePieces(currentSetup);
    } else {
        QMenu contextMenu(this);
        QStringList types = {"Pawn", "Knight", "Bishop", "Rook", "Queen", "King"};

        QMenu *whiteMenu = contextMenu.addMenu("Белые");
        for (const QString &type : types) {
            QIcon icon(QString(":/assets/white_%1.svg").arg(type.toLower()));
            QAction *act = whiteMenu->addAction(icon, type);
            connect(act, &QAction::triggered, [=]() { addPieceAt(row, col, type, "White"); });
        }

        QMenu *blackMenu = contextMenu.addMenu("Чёрные");
        for (const QString &type : types) {
            QIcon icon(QString(":/assets/black_%1.svg").arg(type.toLower()));
            QAction *act = blackMenu->addAction(icon, type);
            connect(act, &QAction::triggered, [=]() { addPieceAt(row, col, type, "Black"); });
        }
        contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::removePieceAt(int row, int col) {
    for (int i = 0; i < currentSetup.size(); ++i) {
        if (currentSetup[i].row == row && currentSetup[i].col == col) {
            currentSetup.removeAt(i);
            break;
        }
    }
}

void MainWindow::addPieceAt(int row, int col, QString type, QString color) {
    removePieceAt(row, col);
    currentSetup.append({type, color, row, col});
    ui->boardView->updatePieces(currentSetup);
}

void MainWindow::onSaveSetupTriggered() {
    if (!isGameInitialized || currentSetup.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Нет расстановки для сохранения!");
        return;
    }

    if (isGameActive) {
        QMessageBox::warning(this, "Ошибка", "Невозможно выполнить во время игры!");
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");

    QDir().mkpath("setups");
    QString filename = "setups/ChessSetup_" + timestamp + ".txt";

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать файл сохранения!");
        return;
    }

    QTextStream out(&file);
    out << "0\n";
    int secs = ui->spinTime->value() * 60;
    out << secs << " " << secs << "\n";

    QString whiteStr, blackStr;
    for (const Piece &p : currentSetup) {
        QString posInfo = QString("%1 %2%3").arg(p.type).arg(QChar('A' + p.col)).arg(8 - p.row);
        if (p.color == "White") whiteStr += posInfo + ", ";
        else blackStr += posInfo + ", ";
    }
    if (!whiteStr.isEmpty()) whiteStr.chop(2);
    if (!blackStr.isEmpty()) blackStr.chop(2);

    out << whiteStr << "\n" << blackStr << "\n";
    file.close();
    QMessageBox::information(this, "Успех", "Сохранено в " + filename);
}

void MainWindow::onLoadSetupTriggered() {
    if (isGameActive) {
        QMessageBox::warning(this, "Ошибка", "Невозможно выполнить во время игры!");
        return;
    }
    QString filePath = QFileDialog::getOpenFileName(this, "Загрузить расстановку", "", "Text Files (*.txt)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    QString firstLine = in.readLine().trimmed();

    if (firstLine != "0") {
        QMessageBox::critical(this, "Ошибка", "Этот файл содержит начатую партию, а не чистую расстановку!");
        file.close();
        return;
    }

    QStringList times = in.readLine().split(" ");
    if(!times.isEmpty()) ui->spinTime->setValue(times[0].toInt() / 60);

    currentSetup.clear();

    for (int i = 0; i < 2; ++i) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QString color = (i == 0) ? "White" : "Black";
        QStringList pList = line.split(", ");

        for (const QString &pData : pList) {
            QStringList tokens = pData.split(" ");
            if (tokens.size() < 2) continue;

            QString type = tokens[0];
            QString pos = tokens[1];

            if(pos.length() < 2) continue;
            int col = pos[0].toUpper().toLatin1() - 'A';
            int row = 8 - QString(pos[1]).toInt();

            currentSetup.append({type, color, row, col});
        }
    }

    file.close();
    isGameInitialized = true;
    menuOverlay->setVisible(false);
    ui->boardView->updatePieces(currentSetup);
}

void MainWindow::performMove(int fromRow, int fromCol, int toRow, int toCol) {
    QString movingType = "";
    for (const Piece &p : currentSetup) {
        if (p.row == fromRow && p.col == fromCol) {
            movingType = p.type;
            break;
        }
    }

    QString promotionType = "";
    if (movingType == "Pawn" && (toRow == 0 || toRow == 7)) {
        QMenu promoMenu(this);
        promoMenu.setTitle("Выберите фигуру");

        QStringList types = {"Queen", "Rook", "Bishop", "Knight"};
        for (const QString &t : types) {
            QString iconPath = QString(":/assets/%1_%2.svg").arg(activePlayer.toLower(), t.toLower());
            QAction *act = promoMenu.addAction(QIcon(iconPath), t);
            act->setData(t);
        }

        QAction *selectedAct = promoMenu.exec(QCursor::pos());
        if (selectedAct) {
            promotionType = selectedAct->data().toString();
        } else {
            promotionType = "Queen";
        }
    }

    QString capturedType = gameEngine->executeMove(fromRow, fromCol, toRow, toCol, promotionType);

    if (!capturedType.isEmpty()) {
        int value = getPieceValue(capturedType);
        if (activePlayer == "White") whiteScore += value;
        else blackScore += value;
        updateScoreUI();
    }

    currentSetup = gameEngine->getActivePieces();

    QString moveText = QString("%1: %2%3 -> %4%5")
        .arg(activePlayer == "White" ? "White" : "Black")
        .arg(QChar('A' + fromCol)).arg(8 - fromRow)
        .arg(QChar('A' + toCol)).arg(8 - toRow);

    if (!promotionType.isEmpty()) {
        moveText += " " + promotionType;
    }

    ui->listMoves->addItem(moveText);
    ui->listMoves->scrollToBottom();

    ui->boardView->updatePieces(currentSetup);
    changeTurn();
}

int MainWindow::getPieceValue(const QString& type) {
    if (type == "Pawn") return 1;
    if (type == "Knight" || type == "Bishop") return 3;
    if (type == "Rook") return 5;
    if (type == "Queen") return 9;
    return 0;
}

void MainWindow::updateScoreUI() {
    int whiteMaterial = gameEngine->getMaterialScore("White");
    int blackMaterial = gameEngine->getMaterialScore("Black");

    QMap<QString, int> capturedWhite;
    QMap<QString, int> capturedBlack;

    for (const Piece& p : initialSetup) {
        if (p.type == "King") continue;
        if (p.color == "White") capturedWhite[p.type]++;
        else capturedBlack[p.type]++;
    }

    QVector<Piece> activePieces = gameEngine->getActivePieces();
    for (const Piece& p : activePieces) {
        if (p.type == "King") continue;
        if (p.color == "White") capturedWhite[p.type]--;
        else capturedBlack[p.type]--;
    }

    auto getCapturedHtml = [](QMap<QString, int>& capturedMap, const QString& colorStr) {
        QString html = "";
        QStringList order = {"Queen", "Rook", "Bishop", "Knight", "Pawn"};

        for (const QString& type : order) {
            int count = capturedMap[type];
            if (count > 0) {
                int iconSize = 20;
                int step = 5;
                int width = iconSize + (count - 1) * step;

                QPixmap pixmap(width, iconSize);
                pixmap.fill(Qt::transparent);
                QPainter painter(&pixmap);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setRenderHint(QPainter::SmoothPixmapTransform);

                QString path = QString(":/assets/%1_%2.svg").arg(colorStr.toLower(), type.toLower());
                QSvgRenderer renderer(path);

                for (int i = 0; i < count; ++i) {
                    renderer.render(&painter, QRectF(i * step, 0, iconSize, iconSize));
                }
                painter.end();

                QByteArray byteArray;
                QBuffer buffer(&byteArray);
                pixmap.save(&buffer, "PNG");
                QString base64 = QString::fromLatin1(byteArray.toBase64());

                html += QString("<img src='data:image/png;base64,%1' style='vertical-align: middle; margin-right: 6px;'/>").arg(base64);
            }
        }
        return html;
    };

    QString whiteText = "Съедено (Белые): " + getCapturedHtml(capturedBlack, "black");
    QString blackText = "Съедено (Чёрные): " + getCapturedHtml(capturedWhite, "white");

    if (whiteMaterial > blackMaterial) {
        whiteText += QString(" +%1").arg(whiteMaterial - blackMaterial);
    } else if (blackMaterial > whiteMaterial) {
        blackText += QString(" +%1").arg(blackMaterial - whiteMaterial);
    }

    ui->lblCapturedWhite->setTextFormat(Qt::RichText);
    ui->lblCapturedBlack->setTextFormat(Qt::RichText);
    ui->lblCapturedWhite->setText(whiteText);
    ui->lblCapturedBlack->setText(blackText);
}

void MainWindow::changeTurn() {
    activePlayer = (activePlayer == "White") ? "Black" : "White";
    selectedRow = -1; selectedCol = -1;
    ui->boardView->clearMoveHints();

    gameEngine->recordPosition(activePlayer);

    if (gameEngine->isFiftyMoveRule()) {
        showPostGameDialog("Конец игры", "Ничья (правило 50 ходов).");
    } else if (gameEngine->isThreefoldRepetition()) {
        showPostGameDialog("Конец игры", "Ничья (троекратное повторение позиции).");
    } else if (gameEngine->isInsufficientMaterial()) {
        showPostGameDialog("Конец игры", "Ничья (недостаточно материала для мата).");
    } else if (gameEngine->isCheckmate(activePlayer)) {
        QString winner = (activePlayer == "White") ? "Чёрные" : "Белые";
        showPostGameDialog("Конец игры", QString("Мат! %1 победили!").arg(winner));
    } else if (gameEngine->isStalemate(activePlayer)) {
        showPostGameDialog("Конец игры", "Пат! Ничья.");
    }
}

void MainWindow::incrementError() {
    ui->boardView->triggerErrorAnimation();

    selectedRow = -1;
    selectedCol = -1;
    ui->boardView->clearMoveHints();

    if (activePlayer == "White") {
        whiteErrors++;
        ui->lblErrorsWhite->setText(QString("Ошибки Белые: %1").arg(whiteErrors));
    } else {
        blackErrors++;
        ui->lblErrorsBlack->setText(QString("Ошибки Чёрные: %1").arg(blackErrors));
    }
}

bool MainWindow::checkIfStandardSetup() {
    if (currentSetup.size() != 32) return false;

    QVector<Piece> stdSetup;
    QStringList order = {"Rook", "Knight", "Bishop", "Queen", "King", "Bishop", "Knight", "Rook"};
    for (int col = 0; col < 8; ++col) {
        stdSetup.append({order[col], "Black", 0, col});
        stdSetup.append({"Pawn", "Black", 1, col});
        stdSetup.append({"Pawn", "White", 6, col});
        stdSetup.append({order[col], "White", 7, col});
    }

    for (const Piece& p1 : stdSetup) {
        bool found = false;
        for (const Piece& p2 : currentSetup) {
            if (p1.type == p2.type && p1.color == p2.color && p1.row == p2.row && p1.col == p2.col) {
                found = true; break;
            }
        }
        if (!found) return false;
    }
    return true;
}

void MainWindow::onEndGameClicked() {
    if (isGameActive) {
        QString winner = (activePlayer == "White") ? "Чёрные" : "Белые";
        QString loser = (activePlayer == "White") ? "Белые" : "Чёрные";
        showPostGameDialog("Игра окончена", QString("%1 сдались! %2 победили.").arg(loser, winner));
    } else if (isPostGame) {
        restoreSetupMode();
    }
}

void MainWindow::showPostGameDialog(const QString& title, const QString& message) {
    gameTimer->stop();
    isGameActive = false;
    isPostGame = true;

    ui->btnPause->setEnabled(false);
    ui->btnContinue->setEnabled(false);
    ui->btnEndGame->setText("К расстановке");

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    QPushButton *btnLook = msgBox.addButton("Взглянуть на поле", QMessageBox::ActionRole);
    QPushButton *btnSetup = msgBox.addButton("Вернуться к расстановке", QMessageBox::ActionRole);
    msgBox.exec();

    if (msgBox.clickedButton() == btnSetup) {
        restoreSetupMode();
    }
}

void MainWindow::restoreSetupMode() {
    isPostGame = false;
    currentSetup = initialSetup;
    ui->boardView->updatePieces(currentSetup);

    ui->btnStart->setEnabled(true);
    ui->btnStandard->setEnabled(true);
    ui->btnCustom->setEnabled(true);
    ui->spinTime->setEnabled(true);

    ui->btnPause->setEnabled(false);
    ui->btnContinue->setEnabled(false);
    ui->btnEndGame->setEnabled(false);
    ui->btnEndGame->setText("Сдаться");

    whiteErrors = 0; blackErrors = 0;
    whiteScore = 0; blackScore = 0;

    ui->listMoves->clear();
    ui->lblErrorsWhite->setText("Ошибки Белые: 0");
    ui->lblErrorsBlack->setText("Ошибки Чёрные: 0");

    ui->lblCapturedWhite->setText("Съедено (Белые):");
    ui->lblCapturedBlack->setText("Съедено (Чёрные):");

    int totalSeconds = ui->spinTime->value() * 60;
    whiteTimeLeft = totalSeconds;
    blackTimeLeft = totalSeconds;
    updateTimerLabels();

    ui->boardView->clearMoveHints();
    if (pauseOverlay->isVisible()) {
        pauseOverlay->setVisible(false);
        isPaused = false;
    }
}

void MainWindow::onSaveGameTriggered() {
    if (!isGameActive && !isPaused && !isPostGame) {
        QMessageBox::warning(this, "Ошибка", "Нет начатой партии для сохранения!");
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");

    QDir().mkpath("games");
    QString filename = "games/ChessGame_" + timestamp + ".txt";

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать файл сохранения!");
        return;
    }

    QTextStream out(&file);

    out << "1\n";

    out << whiteTimeLeft << " " << blackTimeLeft << " " << whiteErrors << " " << blackErrors << "\n";

    QString whiteStr, blackStr;
    for (const Piece &p : initialSetup) {
        QString posInfo = QString("%1 %2%3").arg(p.type).arg(QChar('A' + p.col)).arg(8 - p.row);
        if (p.color == "White") whiteStr += posInfo + ", ";
        else blackStr += posInfo + ", ";
    }
    if (!whiteStr.isEmpty()) whiteStr.chop(2);
    if (!blackStr.isEmpty()) blackStr.chop(2);
    out << whiteStr << "\n" << blackStr << "\n";

    for (int i = 0; i < ui->listMoves->count(); ++i) {
        out << ui->listMoves->item(i)->text() << "\n";
    }

    file.close();
    QMessageBox::information(this, "Успех", "Партия успешно сохранена в " + filename);
}

void MainWindow::onLoadGameTriggered() {
    if (isGameActive) {
        QMessageBox::warning(this, "Ошибка", "Сначала сдайтесь или завершите текущую партию!");
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this, "Загрузить партию", "", "Text Files (*.txt)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    QString firstLine = in.readLine().trimmed();

    if (firstLine != "1") {
        QMessageBox::critical(this, "Ошибка", "Этот файл содержит чистую расстановку, а не начатую партию!");
        file.close();
        return;
    }

    QStringList stats = in.readLine().trimmed().split(" ");
    if (stats.size() >= 4) {
        whiteTimeLeft = stats[0].toInt();
        blackTimeLeft = stats[1].toInt();
        whiteErrors = stats[2].toInt();
        blackErrors = stats[3].toInt();
    }

    initialSetup.clear();
    for (int i = 0; i < 2; ++i) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QString color = (i == 0) ? "White" : "Black";
        QStringList pList = line.split(", ");
        for (const QString &pData : pList) {
            QStringList tokens = pData.split(" ");
            if (tokens.size() < 2) continue;
            QString type = tokens[0];
            QString pos = tokens[1];
            if (pos.length() < 2) continue;
            int col = pos[0].toUpper().toLatin1() - 'A';
            int row = 8 - QString(pos[1]).toInt();
            initialSetup.append({type, color, row, col});
        }
    }

    currentSetup = initialSetup;
    gameEngine->loadSetup(currentSetup);
    gameEngine->setGameMode(checkIfStandardSetup());

    gameEngine->resetDrawCounters();
    gameEngine->recordPosition("White");

    whiteScore = 0; blackScore = 0;
    activePlayer = "White";
    ui->listMoves->clear();

    while (!in.atEnd()) {
        QString moveStr = in.readLine().trimmed();
        if (!moveStr.isEmpty()) {
            replayMove(moveStr);
        }
    }
    file.close();
    menuOverlay->setVisible(false);

    currentSetup = gameEngine->getActivePieces();
    ui->boardView->updatePieces(currentSetup);

    updateScoreUI();
    updateTimerLabels();
    ui->lblErrorsWhite->setText(QString("Ошибки Белые: %1").arg(whiteErrors));
    ui->lblErrorsBlack->setText(QString("Ошибки Чёрные: %1").arg(blackErrors));

    isGameInitialized = true;
    isGameActive = true;
    isPaused = false;
    isPostGame = false;

    ui->btnStart->setEnabled(false);
    ui->btnStandard->setEnabled(false);
    ui->btnCustom->setEnabled(false);
    ui->spinTime->setEnabled(false);

    ui->btnPause->setEnabled(true);
    ui->btnContinue->setEnabled(false);
    ui->btnEndGame->setEnabled(true);
    ui->btnEndGame->setText("Сдаться");

    if (gameEngine->isFiftyMoveRule()) {
        showPostGameDialog("Конец игры", "Ничья (правило 50 ходов).");
    } else if (gameEngine->isThreefoldRepetition()) {
        showPostGameDialog("Конец игры", "Ничья (троекратное повторение позиции).");
    } else if (gameEngine->isInsufficientMaterial()) {
        showPostGameDialog("Конец игры", "Ничья (недостаточно материала для мата).");
    } else if (gameEngine->isCheckmate(activePlayer)) {
        QString winner = (activePlayer == "White") ? "Чёрные" : "Белые";
        showPostGameDialog("Конец игры", QString("Мат! %1 победили!").arg(winner));
    } else if (gameEngine->isStalemate(activePlayer)) {
        showPostGameDialog("Конец игры", "Пат! Ничья.");
    } else {
        gameTimer->start(1000);
        QMessageBox::information(this, "Игра загружена", "Партия успешно восстановлена!");
    }
}

void MainWindow::replayMove(const QString& moveStr) {
    QStringList parts = moveStr.split(": ");
    if (parts.size() < 2) return;

    QString details = parts[1];
    QStringList tokens = details.split(" ");
    if (tokens.size() < 3) return;

    QString fromStr = tokens[0];
    QString toStr = tokens[2];
    QString promotion = (tokens.size() > 3) ? tokens[3] : "";

    int fromCol = fromStr[0].toUpper().toLatin1() - 'A';
    int fromRow = 8 - QString(fromStr[1]).toInt();
    int toCol = toStr[0].toUpper().toLatin1() - 'A';
    int toRow = 8 - QString(toStr[1]).toInt();

    QString capturedType = gameEngine->executeMove(fromRow, fromCol, toRow, toCol, promotion);
    if (!capturedType.isEmpty()) {
        int value = getPieceValue(capturedType);
        if (activePlayer == "White") whiteScore += value;
        else blackScore += value;
    }

    ui->listMoves->addItem(moveStr);

    activePlayer = (activePlayer == "White") ? "Black" : "White";
    gameEngine->recordPosition(activePlayer);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);

    if (menuOverlay) {
        menuOverlay->resize(ui->boardView->size());
    }
    if (pauseOverlay) {
        pauseOverlay->resize(ui->boardView->size());
    }
}
