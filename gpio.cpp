#include "gpio.h"
#include "debug.h"
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>

namespace gpio
{
	vector<Led*> leds;

	template <typename T>
	void writeVal(string fileName, T val)
	{
		ofstream fs;
		fs.open(fileName);
		if (fs.fail())
		{
			cerr << "Failed to open " << fileName << ':' << strerror(errno) << '\n';
			exit(EXIT_FAILURE);
		}
		fs << val;
		fs.close();
	}

	Pin::Pin(int pinNum, int mode)	// initialize a GPIO pin
	{
		int active_low;
		stringstream fileName_s;
		string fileName;

		if (pinNum < 0)	// negative value means active low
		{
			pinNum *= -1;
			active_low = 1;
		}
		else
		{
			active_low = 0;
		}

		fileName_s << "/sys/class/gpio/gpio" << pinNum;
		fileName = fileName_s.str();
		struct stat st;
		if (stat(fileName.c_str(), &st) != 0) writeVal<int>("/sys/class/gpio/export", pinNum);

		writeVal<int>(fileName + "/active_low", active_low);
		writeVal<string>(fileName + "/direction", mode == OUTPUT ? "out" : "in");
		fs.open(fileName + "/value", mode == OUTPUT ? ios::out : ios::in);

		if (fs.fail())
		{
			cerr << "Failed to open " << fileName << ':' << strerror(errno) << '\n';
			exit(EXIT_FAILURE);
		}

		activeText = "";
		inactiveText = "";

		if (debug.verbose) printf("Initialized GPIO pin %d (%sput, active %s)\n", pinNum, mode == OUTPUT ? "out" : "in", active_low ? "low" : "high");
	}

	Pin::~Pin()
	{
		fs.close();
	}

	Led::Led(int pin1, int pin2)
	{
		greenPin = new Pin(pin1, OUTPUT);
		if (pin2 < 65536)
		{
			redPin = new Pin(pin2, OUTPUT);
			biColor = true;
		}
		else biColor = false;

		if (debug.verbose) printf("LED at pin %d is %sbicolor\n", pin1, biColor ? "": "not ");
		
		color = LedOff;
		blink = Solid;
		blinkColor = LedOff;
		blinkState = false;
		setPins(LedOff);
		leds.push_back(this);
	}

	Led::~Led()
	{
		delete greenPin;
		delete redPin;
	}

	void Led::update()
	{
		lock_guard<mutex> guard(m);

		switch (blink)
		{
			case Solid:
				break;

			case BlinkOnce:
			case Blink:
				if (blinkState)
				{
					blinkState = false;
					setPins(color);
					if (blink == BlinkOnce) blink = Solid;
				}
				else
				{
					blinkState = true;
					setPins(blinkColor);
				}
				break;
		}
	}

	void Led::setPins(LedColor c)
	{
		switch (c)
		{
			case LedOff:
				greenPin->set(false);
				if (biColor) redPin->set(false);
				break;

			case Red:
				greenPin->set(false);
				if (biColor) redPin->set(true);
				break;

			case Green:
				greenPin->set(true);
				if (biColor) redPin->set(false);
				break;
		}
	}

	void Led::set(LedColor c, LedBlink b, LedColor bc)
	{
		lock_guard<mutex> guard(m);
		bool update = (color != c);
		color = c;
		blink = b;
		blinkColor = bc;
		if (update) setPins(c);
	}

	void* gpio_thread(void*)
	{
		for (;;)
		{
			usleep(100000);
			
			for (vector<Led*>::iterator l = leds.begin(); l != leds.end(); ++l)
			{
				(*l)->update();
			}
		}
	}
}
