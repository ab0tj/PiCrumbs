#include "gpio.h"
#include "debug.h"
#include <cstdlib>
#include <cstdio>
#include <wiringPi.h>
#include <mcp23008.h>
#include <mcp23017.h>

namespace gpio
{
	bool setupDone;

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
		else
		{
			pin = pinNum;
			active_low = false;
		}

		pullUpDnControl(pin, pullup ? PUD_UP : PUD_OFF);
		if (mode == OUTPUT) digitalWrite(pin, active_low);	// set initial value
		pinMode(pin, mode);

		if (debug.verbose) printf("Initialized GPIO pin %d (%sput, active %s, pullup %s)\n", pin, mode == OUTPUT ? "out" : "in", active_low ? "low" : "high", pullup ? "on" : "off");
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

	void initExpander(ExpanderType type, int address, int pinBase)
	{
		string name;

		switch (type)
		{
			case MCP23008:
				name = "MCP23008";
				mcp23008Setup(pinBase, address);
				break;
			case MCP23017:
				name = "MCP23017";
				mcp23017Setup(pinBase, address);
				break;
		}

		if (debug.verbose) printf("Initialized %s expander at 0x%02X\n", name.c_str(), address);
	}
}
