#pragma once
#include "InputHandler.h"
#include "Config.h"
#include <QtWidgets>
#include <QMainWindow>
#include <QPushButton>
#include <QMenuBar>
#include <QHBoxLayout>
#include <vector>


namespace buttons {

    enum ButtonLabels {
        StartButton = 0,
        RecordButton = 1,
    };
}

class MainWindow : public QMainWindow {

    public:

        MainWindow () : config(),ih(config) {
            this->setIsPlaying(false);
            this->setIsRecording(false);
            setupUI();
            connectSignals();
        }

        void setupUI() {
            Config& config = this->getConfig();
            QWidget *central = new QWidget(this);
            QHBoxLayout *layout = new QHBoxLayout;
            layout->setContentsMargins(20, 20, 20, 20);
            for (int i = 0; i < 2; ++i) {
                QPushButton *btn = new QPushButton();
                btn->setFixedSize(60, 60);
                layout->addWidget(btn);
                switch (i) {
                    case buttons::StartButton: 
                        this->setStartButton(btn);
                        btn->setIcon(QIcon(QString::fromStdString(config.getPlayBtnPath())));
                        btn->setIconSize(QSize(40,40));
                        break;
                    case buttons::RecordButton: 
                        this->setRecordButton(btn); 
                        btn->setIcon(QIcon(QString::fromStdString(config.getRecBtnPath())));
                        btn->setIconSize(QSize(40,40));
                        break;
                    default: 
                        break;
                }
            }
            central->setLayout(layout);
            central->setFixedSize(200, 100);
            setCentralWidget(central);
            setWindowTitle("AutoMacro");
            QMenu *fileMenu = menuBar()->addMenu("File");
            QAction* newAction = fileMenu->addAction("New");
            connect(newAction, &QAction::triggered, this, &MainWindow::newMacroFile);
            QAction* openAction = fileMenu->addAction("Open");
            connect(openAction, &QAction::triggered, this, &MainWindow::openMacroFile);
            fileMenu->addSeparator();
            fileMenu->addAction("Exit", this, SLOT(close()));
            QMenu *settingsMenu = menuBar()->addMenu("Settings");
            QMenu *speedMenu = settingsMenu->addMenu("Macro Speed");
            QAction *speed025 = speedMenu->addAction("0.25x (Very Slow)");
            connect(speed025, &QAction::triggered, [this, &config]() {config.setAndSaveMacroSpeed(0.25f); });
            QAction *speed05 = speedMenu->addAction("0.5x (Slow)");
            connect(speed05, &QAction::triggered, [this, &config]() { config.setAndSaveMacroSpeed(0.5f); });
            QAction *speed1 = speedMenu->addAction("1.0x (Normal)");
            connect(speed1, &QAction::triggered, [this, &config]() { config.setAndSaveMacroSpeed(1.0f); });
            QAction *speed15 = speedMenu->addAction("1.5x (Fast)");
            connect(speed15, &QAction::triggered, [this, &config]() { config.setAndSaveMacroSpeed(1.5f); });
            QAction *speed2 = speedMenu->addAction("2.0x (Very Fast)");
            connect(speed2, &QAction::triggered, [this, &config]() { config.setAndSaveMacroSpeed(2.0f); });
            QAction *speed5 = speedMenu->addAction("5.0x (Ultra Fast)");
            connect(speed5, &QAction::triggered, [this, &config]() { config.setAndSaveMacroSpeed(5.0f); });
            QMenu *loopMenu = settingsMenu->addMenu("Loop Count");
            QAction *loopContinuous = loopMenu->addAction("Continuous (Until Stopped)");
            connect(loopContinuous, &QAction::triggered, [this, &config]() { config.setAndSaveLoopCount(-1); });
            this->setFileMenu(fileMenu);
            this->setLayout(layout);
        }

        void newMacroFile() {
            Config config = this->getConfig();
            if (this->getIsPlaying() || this->getIsRecording()) {
                QMessageBox::warning(this, "Warning", "Cannot create new file while recording or playing macro.");
                return;
            }
            QString defaultPath = QString::fromStdString(config.getMacroSaveFolderRelativePath());
            QString fileName = QFileDialog::getSaveFileName(this,"Create New Macro File", defaultPath + "/newMacro.csv","CSV Files (*.csv);;Text Files (*.txt)");
            if (!fileName.isEmpty()) {
                QFile file(fileName);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    file.close();
                    this->setCurrentMacroPath(fileName.toStdString());
                    config.setAndSaveDefaultFile(fileName.toStdString());
                    QMessageBox::information(this, "Success", 
                        QString("New macro file created: %1").arg(QFileInfo(fileName).fileName()));
                } else {
                    QMessageBox::critical(this, "Error", "Failed to create the file.");
                }
            }
        }

        void openMacroFile() {
            if (this->getIsPlaying() || this->getIsRecording()) {
                QMessageBox::warning(this, "Warning", "Cannot open file while recording or playing macro.");
                return;
            }
            QString defaultPath = QString::fromStdString(config.getMacroSaveFolderRelativePath());
            QString fileName = QFileDialog::getOpenFileName(this,"Open Macro File",defaultPath,"CSV Files (*.csv);;Text Files (*.txt)");
            if (!fileName.isEmpty()) {
                QFile file(fileName);
                if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    file.close();
                    this->setCurrentMacroPath(fileName.toStdString());
                    config.setAndSaveDefaultFile(fileName.toStdString());
                    QMessageBox::information(this, "Success", 
                        QString("Opened macro file: %1").arg(QFileInfo(fileName).fileName()));
                } else {
                    QMessageBox::critical(this, "Error", "Failed to open the file or file doesn't exist.");
                }
            }
        }

        void connectSignals() {
            Config& config = this->getConfig();
            InputHandler& ih = this->getInputHandler();
            QPushButton *startBtn = getStartButton();
            QTimer *shortcutTimer = new QTimer(this);
            connect(shortcutTimer, &QTimer::timeout, [this, &ih] () {
                if (ih.isPlayShortcutPressed() && !this->getIsPlaying()) {
                    this->getStartButton()->click();
                }
                if (ih.isRecordShortcutPressed() && !this->getIsRecording()) {
                    this->getRecordButton()->click();
                }
            });
            shortcutTimer->start(20);
            connect(startBtn, &QPushButton::clicked, [this, startBtn, &config, &ih]() {
                if (!this->getIsPlaying()) {
                    this->setIsPlaying(true);
                    startBtn->setIcon(QIcon(QString::fromStdString(config.getPauseBtnPath())));
                    startBtn->setIconSize(QSize(40,40));
                    startBtn->repaint();
                    QApplication::processEvents();
                    std::this_thread::sleep_for(std::chrono::milliseconds(300));
                    std::thread([this, startBtn, &config, &ih]() {
                        std::atomic<bool> shouldStop{false};
                        std::thread shortcutMonitor([this, &ih, &shouldStop]() {
                            bool wasShortcutPressed = false;
                            while (!shouldStop.load()) {
                                bool isShortcutPressed = ih.isPlayShortcutPressed();
                                if (isShortcutPressed && !wasShortcutPressed) {
                                    ih.requestInterrupt();
                                    break;
                                }
                                wasShortcutPressed = isShortcutPressed;
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            }
                        });
                        ih.playMacro(this->getCurrentMacroFilePath());
                        shouldStop.store(true);
                        if (shortcutMonitor.joinable()) {
                            shortcutMonitor.join();
                        }
                        ih.requestInterrupt();
                        ih.stopInterrupt();
                        QMetaObject::invokeMethod(startBtn, [this, startBtn, &config]() {
                            this->setIsPlaying(false);
                            startBtn->setIcon(QIcon(QString::fromStdString(config.getPlayBtnPath())));
                            startBtn->setIconSize(QSize(40, 40));
                        }, Qt::QueuedConnection);
                    }).detach();
                } else {
                    this->setIsPlaying(false);
                    ih.requestInterrupt();
                    startBtn->setIcon(QIcon(QString::fromStdString(config.getPlayBtnPath())));
                    startBtn->setIconSize(QSize(40,40));
                }
            });
            QPushButton *recordBtn = getRecordButton();
            connect(recordBtn, &QPushButton::clicked, [this, recordBtn, &config, &ih]() {
                if (!this->getIsRecording()) {
                    this->setIsRecording(true);
                    recordBtn->setIcon(QIcon(QString::fromStdString(config.getStopBtnPath())));
                    recordBtn->setIconSize(QSize(40,40));
                    recordBtn->repaint();
                    QApplication::processEvents();
                    std::this_thread::sleep_for(std::chrono::milliseconds(300));
                    std::thread([this, recordBtn, &config, &ih]() {
                        std::atomic<bool> shouldStop{false};
                        std::thread shortcutMonitor([this, &ih, &shouldStop]() {
                            bool wasShortcutPressed = false;
                            while (!shouldStop.load()) {
                                bool isShortcutPressed = ih.isRecordShortcutPressed();
                                if (isShortcutPressed && !wasShortcutPressed) {
                                    ih.requestInterrupt();
                                    break;
                                }
                                wasShortcutPressed = isShortcutPressed;
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            }
                        });
                        ih.recordInputEvents(this->getCurrentMacroFilePath());
                        shouldStop.store(true);
                        if (shortcutMonitor.joinable()) {
                            shortcutMonitor.join();
                        }
                        ih.requestInterrupt();
                        ih.stopInterrupt();
                        QMetaObject::invokeMethod(recordBtn, [this, recordBtn, &config]() {
                            this->setIsRecording(false);
                            recordBtn->setIcon(QIcon(QString::fromStdString(config.getRecBtnPath())));
                            recordBtn->setIconSize(QSize(40, 40));
                        }, Qt::QueuedConnection);
                    }).detach();    
                }
                else {
                    this->setIsRecording(false);
                    ih.requestInterrupt();
                    recordBtn->setIcon(QIcon(QString::fromStdString(config.getRecBtnPath())));
                    recordBtn->setIconSize(QSize(40,40));
                }
            });
        }

        Config& getConfig() {
            return this->config;
        }

        QPushButton* getStartButton () {
            return this->startButton;
        }

        QPushButton* getRecordButton () {
            return this->recordButton;
        }

        InputHandler& getInputHandler() {
            return this->ih;
        }

        bool getIsPlaying() {
            return this->isPlaying;
        }

        bool getIsRecording() {
            return this->isRecording;
        }

        std::string getCurrentMacroFilePath() {
            return this->currentMacroFilePath;
        }

        void setLayout(QHBoxLayout* layout) {
            this->layout = layout;
        }

        void setStartButton (QPushButton* btn) {
            this->startButton = btn;
        }

        void setRecordButton (QPushButton* btn) {
            this->recordButton = btn;
        }
        void setFileMenu (QMenu* fileMenu) {
            this->fileMenu = fileMenu;
        }

        void setIsPlaying (bool isPlaying) {
            this->isPlaying = isPlaying;
        }

        void setIsRecording (bool isRecording) {
            this->isRecording = isRecording;
        }

        void setCurrentMacroPath(std::string currentMacroFilePath) {
            this->currentMacroFilePath = currentMacroFilePath;
        }

    private:

        bool isPlaying;
        bool isRecording;
        std::string currentMacroFilePath;
        QPushButton *startButton, *recordButton;
        QMenu *fileMenu;
        QHBoxLayout* layout;
        Config config;
        InputHandler ih;

};


