#include "gpio.h"
#include <cstdlib>
#include <wiringPi.h>

namespace gpio
{
	namespace
	{
		bool setupDone;
	}

	void initPin(GpioPin& p, int mode)	// initialize a GPIO pin
	{
		if (!setupDone) wiringPiSetup();	// set up wiringPi if we haven't already

		if (p.pin < 0)	// negative value means active low
		{
			p.pin *= -1;
			p.active_low = true;
		}

		pullUpDnControl(p.pin, p.pullup ? PUD_UP : PUD_OFF);
		if (mode == OUTPUT) digitalWrite(p.pin, p.active_low);	// set initial value
		pinMode(p.pin, mode);
		p.enabled = true;
	}

	void setPin(GpioPin p, bool val)	// set GPIO pin to the requested value
	{
		if (!p.enabled) return;
		val ^= p.active_low;			// invert if active low
		digitalWrite(p.pin, val);
	}

	bool readPin(GpioPin p)
	{
		bool val;
		if (!p.enabled) return false;
		val = digitalRead(p.pin);
		val ^= p.active_low;
		return val;
	}
}
