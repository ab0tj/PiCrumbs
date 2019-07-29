#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __PC_GPIO_INC
#define __PC_GPIO_INC

#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <iostream> // debug

#define INPUT   0
#define OUTPUT  1

namespace gpio
{
    enum LedColor { LedOff, Red, Green };
    enum LedBlink { Solid, BlinkOnce, Blink };

    class Pin
    {
        private:
            fstream fs;

        public:
            string activeText;
            string inactiveText;
            inline bool read() { fs.seekp(0); return fs.get() == '1'; }
            inline string read_str() { return read() ? activeText : inactiveText; }
            inline void set(bool val) { fs << (int)val; }
            Pin(int, int);
            ~Pin();
    };

    class Led
    {
        private:
            Pin* greenPin;
            Pin* redPin;
            bool biColor;
            LedColor color;
            LedColor blinkColor;
            LedBlink blink;
            bool blinkState;
            void setPins(LedColor);
            mutex m;

        public:
            void update();
            void set(LedColor, LedBlink = Solid, LedColor = LedOff);
            inline bool isBicolor() { return biColor; }
            inline LedColor getColor() { return color; };
            Led(int, int);
            ~Led();
    };
    extern vector<Led*> leds;

    void* gpio_thread(void*);
};
#endif
