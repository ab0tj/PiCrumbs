#include "gpio.h"
#include <cstdlib>
#include <wiringPi.h>

namespace gpio
{
	bool setupDone;
	GpioExpander expander;

	Pin::Pin(int pinNum, int mode, bool pullup)	// initialize a GPIO pin
	{
		if (!setupDone)
		{
			wiringPiSetup();	// set up wiringPi if we haven't already
			setupDone = true;
		}			

		if (pinNum < 0)	// negative value means active low
		{
			pin = pinNum * -1;
			active_low = true;
		}

		pullUpDnControl(pin, pullup ? PUD_UP : PUD_OFF);
		if (mode == OUTPUT) digitalWrite(pin, active_low);	// set initial value
		pinMode(pin, mode);
	}

	void Pin::set(bool val)	// set GPIO pin to the requested value
	{
		val ^= active_low;			// invert if active low
		digitalWrite(pin, val);
	}

	bool Pin::read()
	{
		bool val;
		val = digitalRead(pin);
		val ^= active_low;
		return val;
	}
}
