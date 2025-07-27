#include <iostream>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets>
#include <vector>
#include "InputHandler.h"
#include "Utils.h"
#include "MainWindow.h"
#include "Config.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}