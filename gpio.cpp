#include "gpio.h"
#include <cstdlib>
#include <wiringPi.h>
#include "beacon.h"
#include "debug.h"

// GLOBAL VARS
extern BeaconStruct beacon;

namespace gpio
{
	bool enabled;			    // can we use gpio pins
    GpioPin hf_en;			    // gpio pin for hf enable
    GpioPin vhf_en;			    // gpio pin for vhf enable
    GpioPin psk_ptt;            // gpio pin to use for psk ptt

	void initPin(GpioPin p, int mode)	// initialize a GPIO pin
	{
		p.enabled = true;

		if (p.pin < 0)
		{
			p.pin *= -1;
			p.active_low = true;
		}

		if (p.type == wiringPi)
		{
			if (mode == OUTPUT) digitalWrite(p.pin, p.active_low ? HIGH : LOW);
			pinMode(p.pin, mode);
			pullUpDnControl(p.pin, p.pullup ? PUD_UP : PUD_OFF);
		}
	}

	void setPin(GpioPin p, bool val)	// set GPIO pin to the requested value
	{
		if (p.type == wiringPi)
		{
			if (val)
			{
				digitalWrite(p.pin, p.active_low ? LOW : HIGH);
			}
			else digitalWrite(p.pin, p.active_low ? HIGH : LOW);
		}
	}

	void init()
	{
		wiringPiSetup();
		if (hf_en.pin < 65536) initPin(hf_en, INPUT);
		if (hf_en.pin < 65536) initPin(vhf_en, INPUT);
		if (psk_ptt.pin < 65536) initPin(psk_ptt, OUTPUT);
	}

	bool isPinActive(GpioPin p)
	{
		if (!p.enabled) return false;

		if (p.type == wiringPi)
		{
			int val = digitalRead(p.pin);
			if ((p.active_low && val == LOW) || (!p.active_low && val == HIGH)) return true;
			return false;
		}

		return false;
	}

	bool check_path(int path) {		// check to see if gpio says we can use this path
		bool ok = true;

		if (!enabled) return true;

		switch (beacon.aprs_paths[path].proto) {
			case 1:
			case 3:
			case 4:
				ok = isPinActive(hf_en);
				if (!ok && debug.fh) printf("FH_DEBUG: HF disabled via GPIO.\n");
				break;
			case 0:
				ok = isPinActive(vhf_en);
				if (!ok && debug.fh) printf("FH_DEBUG: VHF disabled via GPIO.\n");
				break;
		}
		return ok;
	} // END OF 'check_path'
}
