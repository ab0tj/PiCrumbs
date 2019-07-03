#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __PC_GPIO_INC
#define __PC_GPIO_INC

#include <string>

struct GpioPin
{
    bool enabled;
    int pin;
    bool pullup;
    bool active_low;
};

namespace gpio
{
    int get_temp();
    void initPin(GpioPin&, int);
    void setPin(GpioPin, bool);
    bool readPin(GpioPin);
};
#endif
