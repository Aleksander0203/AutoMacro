#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

class Utils {

    public:

        static std::vector<std::string> splitIgnoringContainedInBrackets(std::string string, char delimitier) {
            std::vector<std::string> splitVector = {};
            std::string currentString = "";
            bool inBrackets = false;
            for (int i = 0; i <= string.length(); i++) {
                if ((string[i] == delimitier && (!inBrackets)) || i == string.length()) {
                    splitVector.push_back(currentString);
                    currentString = "";
                }
                else if (string[i] == '(') {
                    currentString += string[i];
                    inBrackets = true;
                }
                else if (string[i] == ')') {
                    currentString += string[i];
                    inBrackets = false;
                }
                else {
                    currentString += string[i];
                }
            }
            return splitVector;
        }

        static std::vector<int> convertStringToCoords (std::string coordsAsString) {
            std::string currentString = "";
            std::vector<int> coords; 
            for (int i = 0; i < coordsAsString.length(); i++) {
                if (coordsAsString[i] == ',' || coordsAsString[i] == ')') {
                    coords.push_back(stoi(currentString));
                    currentString = "";
                }
                else if (coordsAsString[i] != '(' && coordsAsString[i] != ')') {
                    currentString += coordsAsString[i];
                }
            }
            return coords;
        }

        static std::string getStringFromCSV (std::string fileName) {
            std::string finalString;
            std::ifstream file(fileName);
            if (!file.is_open()) {
                std::cerr << "Failed to open file.\n";
                return "";
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            finalString = buffer.str();
            file.close();
            return finalString;
        }

};