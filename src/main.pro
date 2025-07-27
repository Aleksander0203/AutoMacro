QT += widgets

CONFIG += c++17 console
CONFIG -= app_bundle

SOURCES += guiTests.cpp

HEADERS += InputHandler.h Utils.h Config.h MainWindow.h

INCLUDEPATH += /usr/include/X11

LIBS += -lX11 -lXtst -lXi