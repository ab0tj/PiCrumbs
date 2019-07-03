#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __PSK_INC
#define __PSK_INC

// INCLUDES
#include "gpio.h"

// VARS
extern GpioPin pskPttPin;

// FUNCTIONS
void send_psk_aprs(unsigned int, unsigned char, GpioPin, const char*, unsigned char, const char*, unsigned char, const char*);

#endif