#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __PC_GPIO_INC
#define __PC_GPIO_INC

#include <string>

enum GpioPinType { wiringPi, mcp23017 };

struct GpioPin
{
    bool enabled;
    int pin;
    bool pullup;
    bool active_low;
    GpioPinType type;
};

namespace gpio
{
    bool check_path(int);
    int get_temp();
    void init();
    void setPin(GpioPin, bool);
    bool readPin(GpioPin);
    extern bool enabled;			    // can we use gpio pins
    extern GpioPin hf_en;			    // gpio pin for hf enable
    extern GpioPin vhf_en;			    // gpio pin for vhf enable
    extern GpioPin psk_ptt;            // gpio pin to use for psk ptt
};
#endif
