#pragma once
#include <string>
#include <fstream>
#include <chrono>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include "Utils.h"
#include "Config.h"
#include <thread>
#include <atomic>
#include <map>
#include <algorithm>

namespace event {
        
    enum EVENTTYPE {
        MouseMotion,
        MousePressed,
        MouseReleased,
        KeyPressed,
        keyReleased,
    };

}

namespace processes {

    enum ProcessType {
        PlayMacro,
        RecordMacro,
    };

}

namespace keys {

    enum EventKey {
        Ctrl,
        Shift,
        P,
        R,
    };

}

class InputHandler {

    public: 

        InputHandler(Config& config) : config(config){
            this->display = XOpenDisplay(NULL);
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

        bool getIsPlaying() {
            return this->isPlaying;
        }

        bool getIsRecording() {
            this->isRecording;
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

        void setIsRecording(bool isRecording) {
            this->isRecording = isRecording;
        }

        void setIsPlaying(bool isPlaying) {
            this->isPlaying = isPlaying;
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

        void releaseAllInputsUpdated() {
            int result = XGrabKeyboard(this->getDisplay(), DefaultRootWindow(this->getDisplay()),False, GrabModeAsync, GrabModeAsync, CurrentTime);
            if (result == GrabSuccess) {
                XUngrabKeyboard(this->getDisplay(), CurrentTime);
            }
            result = XGrabPointer(this->getDisplay(), DefaultRootWindow(this->getDisplay()),False, 0, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
            if (result == GrabSuccess) {
                XUngrabPointer(this->getDisplay(), CurrentTime);
            }
            XFlush(this->getDisplay());
        }
        
        void recordInputEvents(std::string filePath) {
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
            bool firstReleaseBlocker = true;
            std::map<int, long long> lastKeyEventTime;
            std::map<int, int> rapidEventCount;
            const long long POLLING_THRESHOLD = 100000;
            const int MAX_RAPID_EVENTS = 5;
            auto recordingStart = std::chrono::system_clock::now();
            const long long STARTUP_DELAY = 200000;
            while (!interruptFlag) {
                XNextEvent(display, &event);
                if (event.xcookie.type == GenericEvent && XGetEventData(display, &event.xcookie)) {
                    long long currentTime = this->getTimestamp(start);
                    if (currentTime < STARTUP_DELAY) {
                        XFreeEventData(display, &event.xcookie);
                        continue;
                    }
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
                            if (firstReleaseBlocker) {
                                firstReleaseBlocker = !firstReleaseBlocker;
                                XFreeEventData(display, &event.xcookie);
                                continue;
                            }
                            else {
                                eventType = event::MouseReleased; 
                            }
                        } 
                        else if (event.xcookie.evtype == XI_RawKeyPress) {
                            eventType = event::KeyPressed;
                        } 
                        else {
                            eventType = event::keyReleased;
                        }
                        XIRawEvent* rawEvent = (XIRawEvent *) event.xcookie.data;
                        int keyCode = rawEvent->detail;
                        long long timeStamp = this->getTimestamp(start);
                        bool isPollingEvent = false;
                        if (lastKeyEventTime.count(keyCode)) {
                            long long timeDiff = timeStamp - lastKeyEventTime[keyCode];
                            if (timeDiff < POLLING_THRESHOLD) {
                                rapidEventCount[keyCode]++;
                                if (rapidEventCount[keyCode] > MAX_RAPID_EVENTS) {
                                    isPollingEvent = true;
                                }
                            } else {
                                rapidEventCount[keyCode] = 0;
                            }
                        } else {
                            rapidEventCount[keyCode] = 0;
                        }
                        lastKeyEventTime[keyCode] = timeStamp;
                        if (isPollingEvent) {
                            XFreeEventData(display, &event.xcookie);
                            continue;
                        }
                        KeySym keysym = XkbKeycodeToKeysym(display, keyCode, 0, 0);
                        bool isHotkeyPoll = (keysym == XK_Control_L || keysym == XK_Control_R || 
                                        keysym == XK_Shift_L || keysym == XK_Shift_R ||
                                        keysym == XK_P || keysym == XK_p ||
                                        keysym == XK_R || keysym == XK_r) && 
                                        rapidEventCount[keyCode] > 2;
                        if (isHotkeyPoll) {
                            XFreeEventData(display, &event.xcookie);
                            continue;
                        }
                        saveString += std::to_string(eventType) + "," + std::to_string(keyCode) + "," + std::to_string(timeStamp) + "\n";
                    }
                    eventCounter++;
                    XFreeEventData(display, &event.xcookie);
                }
            }
            this->stopInterrupt();
            saveString = this->removeUnreleasedPressesFromSave(saveString);
            this->saveEvent(saveString, filePath);
        }

        long long getTimestamp (std::chrono::time_point<std::chrono::system_clock> start) {
            auto current = std::chrono::system_clock::now();
            long long currentSinceEpoch = std::chrono::duration_cast<std::chrono::microseconds>(current.time_since_epoch()).count();
            long long startSinceEpoch = std::chrono::duration_cast<std::chrono::microseconds>(start.time_since_epoch()).count();
            long long durationSinceStart = currentSinceEpoch - startSinceEpoch;
            return durationSinceStart;
        }

        void saveEvent (std::string saveString, std::string filePath) {
            std::ofstream file(filePath);
            file << saveString;
            file.close();
            return;
        }

        void playMacroOnce (std::string fileName) {
            Utils utils;
            std::string fileString = utils.getStringFromCSV(fileName);
            std::istringstream stream(fileString);
            std::string line;
            long long timeDiff = 0; 
            long long time = 0;
            long long prevTime = 0;
            long long sleepTime = 0;
            while (std::getline(stream, line)) {
                std::vector<std::string> eventVector;
                eventVector = utils.splitIgnoringContainedInBrackets(line, ',');
                prevTime = time;
                time = stol(eventVector[2]);
                timeDiff = time - prevTime;
                sleepTime = (long long) timeDiff / this->config.getMacroSpeed();
                std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
                if (interruptFlag) {
                    this->requestInterrupt();
                    break;
                }
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
                }
                else if (stoi(eventVector[0]) == event::keyReleased) {
                    int keyCode = stoi(eventVector[1]);
                    this->pressKey(false, keyCode);
                }
            }
            std::cout << "Macro loop ended\n";
            this->releaseAllInputs();
            return;
        }

        void playMacro(std::string fileName) {
            int loopCount = this->config.getLoopCount();
            if (loopCount == -1) {
                while(true && !interruptFlag) {
                    playMacroOnce(fileName);
                }
            }
            else {
                for (int i = 0; i < loopCount && !interruptFlag; i++) {
                    playMacroOnce(fileName);
                }
            }
            this->stopInterrupt();

        }

        bool isPlayShortcutPressed() {
            Display* display = this->getDisplay();
            char keysReturn[32];
            XQueryKeymap(display, keysReturn);
            KeyCode ctrlKey = XKeysymToKeycode(display, XK_Control_L);
            KeyCode shiftKey = XKeysymToKeycode(display, XK_Shift_L);
            KeyCode pKey = XKeysymToKeycode(display, XK_P);
            return (keysReturn[ctrlKey >> 3] & (1 << (ctrlKey & 7))) && (keysReturn[shiftKey >> 3] & (1 << (shiftKey & 7))) && (keysReturn[pKey >> 3] & (1 << (pKey & 7)));
        }

        bool isRecordShortcutPressed() {
            Display* display = this->getDisplay();
            char keysReturn[32];
            XQueryKeymap(display, keysReturn);
            KeyCode ctrlKey = XKeysymToKeycode(display, XK_Control_L);
            KeyCode shiftKey = XKeysymToKeycode(display, XK_Shift_L);
            KeyCode rKey = XKeysymToKeycode(display, XK_R);
            return (keysReturn[ctrlKey >> 3] & (1 << (ctrlKey & 7))) && (keysReturn[shiftKey >> 3] & (1 << (shiftKey & 7))) && (keysReturn[rKey >> 3] & (1 << (rKey & 7)));
        }

        std::string removeUnreleasedPressesFromSave(std::string saveString) {
            Utils utils;
            std::map<int, bool> keyPressed;
            std::map<int, int> lineOfLastPressOfKeys;
            std::istringstream firstStream(saveString);
            std::ostringstream output;
            std::string line;
            int lineNum = 0;
            while (std::getline(firstStream,line)) {
                std::vector<std::string> eventVector = utils.splitIgnoringContainedInBrackets(line, ',');
                if (std::stoi(eventVector[0]) == event::KeyPressed) {
                    keyPressed[std::stoi(eventVector[1])] = true;
                    lineOfLastPressOfKeys[std::stoi(eventVector[1])] = lineNum;
                } 
                else if (std::stoi(eventVector[0]) == event::keyReleased) {
                    keyPressed[std::stoi(eventVector[1])] = false;
                }
                lineNum++;  
            }
            lineNum = 0;
            std::vector<int> linesToRemove;
            for (auto key: keyPressed) {
                linesToRemove.push_back(lineOfLastPressOfKeys[key.first]);
            }
            std::istringstream secondStream(saveString);
            while (std::getline(secondStream,line)) {
                if (std::find(linesToRemove.begin(), linesToRemove.end(), lineNum) == linesToRemove.end())
                    output << line << '\n';
                ++lineNum;
            }
            return output.str();
        }

        void requestInterrupt() {
            interruptFlag = true;
        }

        void stopInterrupt() {
            interruptFlag = false;
        }
        

    private:

        int x;
        int y;
        Config& config;
        Display* display;
        bool isPlaying = false;
        bool isRecording = false;
        std::atomic<bool> interruptFlag = {false};

};