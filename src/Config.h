#pragma once
#include <unistd.h>
#include <iostream>
#include <limits.h>
#include <filesystem>
#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <QtWidgets>

namespace loops {
    enum LoopAmount {
        Continuous = -1,
    };
}

namespace settings{
    enum ConfigSettings {
        DefaultFile = 0,
        MacroSpeed = 1,
        LoopCount = 2,
    };
}

class Config {

    public:
        
        Config() {
            std::string configString = this->createAndOrReadConfigFile();
            std::istringstream stream(configString);
            std::string line;
            int loopCounter = 0;
            while (std::getline(stream,line)) {
                size_t equalsIndex = line.find('=');
                if (loopCounter == settings::DefaultFile) {
                    if (line.length() == equalsIndex + 1) {
                        loopCounter++;
                        continue;
                    }
                    else {
                        std::string setting = line.substr(equalsIndex + 1);
                        this->setDefaultFile(setting);
                    }
                } 
                else if (loopCounter == settings::MacroSpeed) {
                    std::string setting = line.substr(equalsIndex+1);
                    this->setMacroSpeed(std::stof(setting));
                }
                else if (loopCounter == settings::LoopCount) {
                    std::string setting = line.substr(equalsIndex+1);
                    this->setLoopCount(stoi(setting));
                }
                loopCounter++;
            }
            this->setImagePaths();
            std::cout <<  this->getPauseBtnPath() + "\n"; 
            std::cout <<  this->getPlayBtnPath() + "\n"; 
            std::cout <<  this->getStopBtnPath() + "\n"; 
            std::cout <<  this->getRecBtnPath() + "\n"; 
        }
        
        std::string createAndOrReadConfigFile() {
            char result[PATH_MAX];
            std::string executableDir;
            ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
            if (count == -1) {
                std::cerr << "Failed to get executable path\n";
                return "";
            }
            executableDir = std::filesystem::path(std::string(result, count)).parent_path().string();
            std::string configDir = executableDir + "/config";
            std::filesystem::create_directories(configDir);
            std::string filePath = configDir + "/config.txt";
            std::ifstream file(filePath);
            std::string fileString;
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                fileString = buffer.str();
                file.close();
            }
            else {
                file.close();
                fileString = this->getDefaultConfig();
                std::ofstream fileOutput(filePath); 
                fileOutput << fileString;
            }
            return fileString;
        }

        std::string getMacroSaveFolderRelativePath() {
            char result[PATH_MAX];
            std::string executableDir;
            ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
            if (count == -1) {
                std::cerr << "Failed to get executable path\n";
                return "";
            }
            executableDir = std::filesystem::path(std::string(result, count)).parent_path().string();
            std::string macroFolderDir = executableDir + "/macros/";
            return macroFolderDir;
        }

        std::string getImageFolderRelativePath() {
            char result[PATH_MAX];
            std::string executableDir;
            ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
            if (count == -1) {
                std::cerr << "Failed to get executable path\n";
                return "";
            }
            executableDir = std::filesystem::path(std::string(result, count)).parent_path().string();
            std::string imageFolderDir = executableDir + "/images/";
            return imageFolderDir;
        }

        std::string getConfigFileRelativePath() {
            char result[PATH_MAX];
            std::string executableDir;
            ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
            if (count == -1) {
                std::cerr << "Failed to get executable path\n";
                return "";
            }
            executableDir = std::filesystem::path(std::string(result, count)).parent_path().string();
            std::string configDir = executableDir + "/config/config.txt";
            return configDir;
        }

        void autoSaveConfigFile() {
            std::string stringLoopCount = std::to_string(this->getLoopCount());
            std::string stringMacroSpeed = std::to_string(this->getMacroSpeed());
            std::string defaultFile = this->getDefaultFile();
            std::ofstream file(this->getConfigFileRelativePath());
            file << "defaultFile=" + defaultFile + "\nmacroSpeed=" + stringMacroSpeed + "\nloopCount=" + stringLoopCount;            
        }

        std::string getDefaultConfig() {
            return this->defaultConfig;
        }

        std::string getDefaultFile() {
            return this->defaultFile;
        }

        float getMacroSpeed() {
            return this->macroSpeed;
        }

        int getLoopCount() {
            return this->loopCount;
        }

        std::string getPlayBtnPath() {
            return this->playBtnPath;
        }

        std::string getPauseBtnPath() {
            return this->pauseBtnPath;
        }

        std::string getRecBtnPath() {
            return this->recBtnPath;
        }

        std::string getStopBtnPath() {
            return this->stopBtnPath;
        }

        void setImagePaths () {
            std::string imageFolderPath = this->getImageFolderRelativePath();
            this->playBtnPath = imageFolderPath + "PlayBtn.png";
            this->recBtnPath = imageFolderPath + "RecBtn.png";
            this->pauseBtnPath = imageFolderPath + "PauseBtn.png";
            this->stopBtnPath = imageFolderPath + "StopBtn.png";
        }

        void setDefaultFile(std::string defaultFile) {
            this->defaultFile = defaultFile;
        }

        void setMacroSpeed(float macroSpeed) {
            this->macroSpeed = macroSpeed;
        }

        void setLoopCount(int loopCount) {
            this->loopCount = loopCount;
        }

        void setAndSaveDefaultFile(std::string defaultFile) {
            this->defaultFile = defaultFile;
            this->autoSaveConfigFile();
        }

        void setAndSaveMacroSpeed(float macroSpeed) {
            this->macroSpeed = macroSpeed;
            this->autoSaveConfigFile();
        }

        void setAndSaveLoopCount(int loopCount) {
            this->loopCount = loopCount;
            this->autoSaveConfigFile();
        }

    private:

        const std::string defaultConfig = "defaultFile=\nmacroSpeed=1.0\nloopCount=-1";
        const std::string configFilePath;
        std::string defaultFile;
        std::string imagesPath = "";
        float macroSpeed;
        int loopCount;
        const std::string playMacroHotkey = "Ctrl+Shift+P";
        const std::string recordMacroHotkey = "Ctrl+Shift+R";
        std::string playBtnPath, pauseBtnPath, recBtnPath, stopBtnPath;

};