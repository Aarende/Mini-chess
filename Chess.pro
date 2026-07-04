QT       += core gui widgets svg svgwidgets

TARGET = MiniChess
TEMPLATE = app

SOURCES += \
    chessgame.cpp \
    main.cpp \
    mainwindow.cpp \
    chessboardview.cpp

HEADERS += \
    chessgame.h \
    mainwindow.h \
    chessboardview.h \
    chessdata.h

FORMS += \
    mainwindow.ui

RC_ICONS = chess.ico

RESOURCES += \
    resources.qrc

win32: QMAKE_LFLAGS += \
    -static \
    -static-libgcc \
    -static-libstdc++
