#include "pi.h"
#include <string>
#include <fstream>
#include <cstdlib>
#include <wiringPi.h>
#include "beacon.h"
#include "debug.h"

// GLOBAL VARS
extern BeaconStruct beacon;
extern DebugStruct debug;
// VARS
string temp_file;						// file to get 1-wire temp info from, blank to disable
bool temp_f;							// temp units: false for C, true for F
unsigned char gpio_hf_en;				// gpio pin for hf enable
unsigned char gpio_vhf_en;				// gpio pin for vhf enable
unsigned char gpio_psk_ptt;				// gpio pin to use for psk ptt

bool check_gpio(int path) {		// check to see if gpio says we can use this path
	bool ok = true;
	switch (beacon.aprs_paths[path].proto) {
		case 1:
		case 3:
		case 4:
			ok = (digitalRead(gpio_hf_en) == 0);
			if (!ok && debug.fh) printf("FH_DEBUG: HF disabled via GPIO.\n");
			break;
		case 0:
			ok = (digitalRead(gpio_vhf_en) == 0);
			if (!ok && debug.fh) printf("FH_DEBUG: VHF disabled via GPIO.\n");
			break;
	}
	return ok;
} // END OF 'check_gpio'

int get_temp() {	// get temperature from one-wire sensor
	ifstream tempfile;
	string line;
	int temp = 0;
	tempfile.open(temp_file.c_str());
	if (!tempfile.is_open()) return -65535;		// return absurdly low value so we know it's invalid.
	getline(tempfile, line);
	if (line.substr(36,3).compare("YES") == 0) {
		getline(tempfile, line, '=');
		getline(tempfile, line);
		temp = atoi(line.c_str());
		if (temp_f) temp = 1.8*temp+32000;
		temp /= 1000;
	} else temp = -65535;
	tempfile.close();
	return temp;
} // END OF 'get_temp'
