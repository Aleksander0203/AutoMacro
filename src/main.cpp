#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <list>
#include <X11/Xlib.h>
#include <unistd.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <format>


class MouseHandler {

    private:

        int x;
        int y;
        Display* display;
        bool pressed;

    public: 

        MouseHandler(Display *display) {
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
        
        void recordMouseEvent() {
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
            while (true) {
                XNextEvent(display, &event);
                if (event.xcookie.type == GenericEvent  && XGetEventData(display, &event.xcookie)) {                    
                    if (event.xcookie.evtype == XI_RawMotion) {
                        this->setMouseCoordinates();
                        std::string message = "Mouse moved to (" + std::to_string(this->getX()) + "," + std::to_string(this->getY()) + ")\n";
                        std::cout << message;
                    } else if (event.xcookie.evtype == XI_RawButtonPress) {
                        std::cout << "Mouse button pressed\n";
                    } else if (event.xcookie.evtype == XI_RawButtonRelease) {
                        std::cout << "Mouse button released\n";
                    } else if (event.xcookie.evtype == XI_RawKeyPress) {
                        std::cout << "Key pressed\n";
                    } else if (event.xcookie.evtype == XI_RawKeyRelease) {
                        XIRawEvent* rawEvent = (XIRawEvent*) event.xcookie.data;
                        int keyCode = rawEvent->detail;
                        std::cout << "Key " + std::to_string(keyCode) +" released\n";
                    }
                    XFreeEventData(display, &event.xcookie);
                }
            }

        } 

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
    MouseHandler mh(display);
    mh.recordMouseEvent();
    return 0;
}