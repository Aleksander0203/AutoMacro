#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <list>
#include <X11/Xlib.h>
#include <unistd.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <format>
#include <vector>
#include <thread>

    namespace event {
        
        enum EVENTTYPE {
            MouseMotion,
            MousePressed,
            MouseReleased,
            KeyPressed,
            keyReleased,
        };

    }

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

};

class InputHandler {

    public: 

        InputHandler(Display* display) {
            this->display = display;
            setMouseCoordinates();    
        }

        void setMouseCoordinates() {
            XButtonEvent event;
            XQueryPointer(this->getDisplay(), DefaultRootWindow(this->getDisplay()), &event.root, &event.window, &event.x_root, &event.y_root, &event.x, &event.y, &event.state);
            setX(event.x);
            setY(event.y);
        }

        void moveMouseTo(int x, int y) {
            setMouseCoordinates();
            XWarpPointer(this->getDisplay(), None, None, 0, 0, 0, 0, x-this->getX(), y-this->getY());
            XFlush(display);
        }

        void clickMouse(bool toPress, int mouseButton) {
            XTestFakeButtonEvent(this->getDisplay(), mouseButton, toPress, CurrentTime);
            XFlush(this->getDisplay());
        }

        void pressKey(bool toPress, int keyCode) {
            XTestFakeKeyEvent(this->getDisplay(), keyCode, toPress, CurrentTime);
            XFlush(this->getDisplay());
        }

        void setX(int x) {
            this->x = x;
        }

        void setY(int y) {
            this->y = y;
        }
        int getX() {
            return this->x;
        }
        int getY() {
            return this->y;
        }

        Display* getDisplay() {
            return this->display;
        }

        void releaseAllKeys() {
            const bool RELEASE = false;
            int minKeyCode, maxKeyCode;
            XDisplayKeycodes(this->getDisplay(), &minKeyCode, &maxKeyCode);
            for (int keyCode = minKeyCode; keyCode <= maxKeyCode; keyCode++) {
                this->pressKey(RELEASE, keyCode);
            }
            XFlush(this->getDisplay());
        }

        void releaseAllMouseButtons () {
            const bool RELEASE = false;
            unsigned char map[256];
            int numButtons = XGetPointerMapping(display, map, sizeof(map));
            for (int mouseButton = 1; mouseButton < numButtons; ++mouseButton) {
                this->clickMouse(RELEASE, mouseButton);
            }
        }

        void releaseAllInputs () {
            releaseAllKeys();
            releaseAllMouseButtons();
        }
        
        void recordInputEvent() {
            Display* display = this->getDisplay();
            XIEventMask eventMask;
            unsigned char mask[(XI_LASTEVENT+7)/8] = {0};
            eventMask.deviceid = XIAllMasterDevices;
            eventMask.mask_len = sizeof(mask);
            eventMask.mask = mask;
            XISetMask(mask, XI_RawMotion);
            XISetMask(mask, XI_RawButtonPress);
            XISetMask(mask, XI_RawButtonRelease);
            XISetMask(mask, XI_RawKeyPress);
            XISetMask(mask, XI_RawKeyRelease);
            XISelectEvents(display, DefaultRootWindow(display), &eventMask, 1);
            XFlush(display);
            XEvent event;
            std::string saveString = "";
            std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
            int eventCounter = 0;
            while (eventCounter < 10000) {
                XNextEvent(display, &event);
                if (event.xcookie.type == GenericEvent  && XGetEventData(display, &event.xcookie)) {                    
                    if (event.xcookie.evtype == XI_RawMotion) {
                        this->setMouseCoordinates();
                        long long timeStamp = this->getTimestamp(start);
                        saveString += std::to_string(event::MouseMotion) + ",(" + std::to_string(this->getX()) + "," + std::to_string(this->getY()) +")," + std::to_string(timeStamp) + "\n";
                    }
                    else {
                        event::EVENTTYPE eventType;
                        if (event.xcookie.evtype == XI_RawButtonPress) {
                            eventType = event::MousePressed;
                        }
                        else if (event.xcookie.evtype == XI_RawButtonRelease) {
                            eventType = event::MouseReleased;
                        } 
                        else if (event.xcookie.evtype == XI_RawKeyPress) {
                            eventType = event::KeyPressed;
                        } 
                        else {
                            eventType = event::keyReleased;
                        }
                        XIRawEvent* rawEvent = (XIRawEvent *) event.xcookie.data;
                        long long timeStamp = this->getTimestamp(start);
                        int keyCode = rawEvent->detail;
                        saveString += std::to_string(eventType) + "," + std::to_string(keyCode) + "," + std::to_string(timeStamp) + "\n";
                    }
                    eventCounter++;
                    XFreeEventData(display, &event.xcookie);
                }
            }
            this->saveEvent(saveString);

        } 

        long long getTimestamp (std::chrono::time_point<std::chrono::system_clock> start) {
            auto current = std::chrono::system_clock::now();
            long long currentSinceEpoch = std::chrono::duration_cast<std::chrono::microseconds>(current.time_since_epoch()).count();
            long long startSinceEpoch = std::chrono::duration_cast<std::chrono::microseconds>(start.time_since_epoch()).count();
            long long durationSinceStart = currentSinceEpoch - startSinceEpoch;
            return durationSinceStart;
        }

        void saveEvent (std::string saveString) {
            std::ofstream file("test1.txt");
            file << saveString;
            file.close();
            return;
        }

        void playMacro (std::string fileName) {
            std::ifstream file(fileName);
            std::string line;
            Utils utils;
            long long timeDiff = 0; 
            long long time = 0;
            long long prevTime = 0;
            while (file.is_open()) {
                while (std::getline(file, line)) {
                    std::vector<std::string> eventVector;
                    eventVector = utils.splitIgnoringContainedInBrackets(line, ',');
                    prevTime = time;
                    time = stol(eventVector[2]);
                    timeDiff = time - prevTime;
                    std::this_thread::sleep_for(std::chrono::microseconds(timeDiff));
                    if (stoi(eventVector[0]) == event::MouseMotion) {
                        std::vector<int> mouseCoords = utils.convertStringToCoords(eventVector[1]);
                        this->moveMouseTo(mouseCoords[0],mouseCoords[1]);
                    }
                    else if (stoi(eventVector[0]) == event::MousePressed) {
                        int mouseButton = stoi(eventVector[1]);
                        this->clickMouse(true, mouseButton);
                    }
                    else if (stoi(eventVector[0]) == event::MouseReleased) {
                        int mouseButton = stoi(eventVector[1]);
                        this->clickMouse(false,mouseButton);
                    }
                    else if (stoi(eventVector[0]) == event::KeyPressed) {
                        int keyCode = stoi(eventVector[1]);
                        this->pressKey(true, keyCode);
                        break;
                    }
                    else if (stoi(eventVector[0]) == event::keyReleased) {
                        int keyCode = stoi(eventVector[1]);
                        this->pressKey(true, keyCode);
                        break;
                    }
                    else {
                        file.close();
                        this->releaseAllInputs();
                    }
                }
            }
            return;
        }

    private:

        int x;
        int y;
        Display* display;


};


int main() {
    Display* display = XOpenDisplay(NULL);
    InputHandler ih(display);
    ih.playMacro("test1.csv");
    return 0;
}