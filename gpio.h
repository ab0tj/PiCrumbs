#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __PC_GPIO_INC
#define __PC_GPIO_INC

#include <string>
#include <vector>
#include <mutex>

namespace gpio
{
    enum ExpanderType { MCP23008 = 0, MCP23017 = 1 };
    enum LedColor { LedOff, Red, Green };
    enum LedBlink { Solid, BlinkOnce, Blink };

    class Pin
    {
        private:
            int pin;
            bool active_low;

        public:
            string activeText;
            string inactiveText;
            void set(bool);
            bool read();
            inline string read_str() { return read() ? activeText : inactiveText; }
            Pin(int, int, bool);
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

    void initExpander(ExpanderType, int, int);
    void* gpio_thread(void*);
};
#endif
