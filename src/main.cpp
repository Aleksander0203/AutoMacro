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

namespace Keys{

            enum KEYSFORKEYCODE {
                //Letters
                A = XK_A,
                B = XK_B,
                C = XK_C,
                D = XK_D,
                E = XK_E,
                F = XK_F,
                G = XK_G,
                H = XK_H,
                I = XK_I,
                J = XK_J,
                K = XK_K,
                L = XK_L,
                M = XK_M,
                N = XK_N,
                O = XK_O,
                P = XK_P,
                Q = XK_Q,
                R = XK_R,
                S = XK_S,
                T = XK_T,
                U = XK_U,
                V = XK_V,
                W = XK_W,
                X = XK_X,
                Y = XK_Y,
                Z = XK_Z,

                //Nums
                KEY_0 = XK_0,
                KEY_1 = XK_1,
                KEY_2 = XK_2,
                KEY_3 = XK_3,
                KEY_4 = XK_4,
                KEY_5 = XK_5,
                KEY_6 = XK_6,
                KEY_7 = XK_7,
                KEY_8 = XK_8,
                KEY_9 = XK_9,

                //Fn
                F1  = XK_F1,
                F2  = XK_F2,
                F3  = XK_F3,
                F4  = XK_F4,
                F5  = XK_F5,
                F6  = XK_F6,
                F7  = XK_F7,
                F8  = XK_F8,
                F9  = XK_F9,
                F10 = XK_F10,
                F11 = XK_F11,
                F12 = XK_F12,

                //modifiers
                Shift_L = XK_Shift_L,
                Shift_R = XK_Shift_R,
                Control_L = XK_Control_L,
                Control_R = XK_Control_R,
                Alt_L = XK_Alt_L,
                Alt_R = XK_Alt_R,
                Super_L = XK_Super_L,
                Super_R = XK_Super_R,
                Meta_L = XK_Meta_L,
                Meta_R = XK_Meta_R,

                //Special keys
                Escape = XK_Escape,
                Tab = XK_Tab,
                Return = XK_Return,
                Backspace = XK_BackSpace,
                Space = XK_space,
                Insert = XK_Insert,
                Delete = XK_Delete,
                Home = XK_Home,
                End = XK_End,
                Page_Up = XK_Page_Up,
                Page_Down = XK_Page_Down,
                Left = XK_Left,
                Right = XK_Right,
                Up = XK_Up,
                Down = XK_Down,
                Caps_Lock = XK_Caps_Lock,
                Num_Lock = XK_Num_Lock,
                Scroll_Lock = XK_Scroll_Lock,
                Minus = XK_minus,
                Equal = XK_equal,
                Bracket_Left = XK_bracketleft,
                Bracket_Right = XK_bracketright,
                Semicolon = XK_semicolon,
                Apostrophe = XK_apostrophe,
                Grave = XK_grave,
                Backslash = XK_backslash,
                Comma = XK_comma,
                Period = XK_period,
                Slash = XK_slash,
            };
        }

    namespace event {
        
        enum EVENTTYPE {
            MouseMotion,
            MousePressed,
            MouseReleased,
            KeyPressed,
            keyReleased,
        };

    }

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

        void clickMouse(bool toPress) {
            XTestFakeButtonEvent(this->getDisplay(), 1, toPress, CurrentTime);
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

        bool getPressed() {
            return this->pressed; 
        }

        void setPressed(bool pressed) {
            this->pressed = pressed;
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
            while (eventCounter < 100) {
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
            long long currentSinceEpoch = std::chrono::duration_cast<std::chrono::nanoseconds>(current.time_since_epoch()).count();
            long long startSinceEpoch = std::chrono::duration_cast<std::chrono::nanoseconds>(start.time_since_epoch()).count();
            long long durationSinceStart = currentSinceEpoch - startSinceEpoch;
            return durationSinceStart;
        }

        void saveEvent (std::string saveString) {
            std::ofstream file("test1.txt");
            file << saveString;
            file.close();
            return;
        }

    private:

        int x;
        int y;
        Display* display;
        bool pressed;

};


class Main {

    void sendKeyTest(Display *display, int keyCode, bool toPress) {
        XTestGrabControl(display,True);
        KeyCode keycode = XKeysymToKeycode(display, keyCode);
            XTestFakeKeyEvent(display,keycode,toPress,CurrentTime);
        XFlush(display);
        return;
    }

};

int main() {
    Display* display = XOpenDisplay(NULL);
    InputHandler ih(display);
    ih.recordInputEvent();
    return 0;
}