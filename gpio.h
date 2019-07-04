#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __PC_GPIO_INC
#define __PC_GPIO_INC

#include <string>

namespace gpio
{
    enum ExpanderType { MCP23008 = 0, MCP23017 = 1 };

    class Pin
    {
        private:
            bool enabled;
            int pin;
            bool active_low;

        public:
            void set(bool);
            bool read();
            Pin(int, int, bool);
    };

    struct GpioExpander
    {
        bool enabled;
        int pinBase;
        int address;
        ExpanderType type;
    };

    extern GpioExpander expander;

    int get_temp();
};
#endif
