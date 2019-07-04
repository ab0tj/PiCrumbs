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
            int pin;
            bool active_low;

        public:
            void set(bool);
            bool read();
            Pin(int, int, bool);
    };

    int get_temp();
    void initExpander(ExpanderType, int, int);
};
#endif
