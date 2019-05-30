#include "hamlib.h"
#include <unistd.h>
#include "beacon.h"

// GLOBAL VARS
extern vector<aprspath> aprs_paths;
extern bool hl_debug;

// LOCAL VARS
Rig* radio;								// radio control interface reference
bool hamlib_enable;						// is radio control enabled?

bool tune_radio(int path) {		// use hamlib to tune the radio to the freq and mode of this path
	if (!hamlib_enable) return true;	// don't try to do tuning stuff if we can't

	try {
		if (radio->getFreq() == aprs_paths[path].freq) return true; // skip if already set
		if (hl_debug) printf("HL_DEBUG: Tuning radio to %.0f\n", aprs_paths[path].freq);
		radio->setFreq(Hz(aprs_paths[path].freq));
		radio->setMode(aprs_paths[path].mode);
		sleep(2);	// let the radio do it's thing. not all of them tune instantly
		return true;
	} catch (const RigException& e) {
		fprintf(stderr, "Hamlib error %i: %s\n", e.errorno, e.message);
		return false;
	}
	return false;		// should never hit this, but we must always return something
}	// END OF 'tune_radio'

freq_t get_radio_freq() {	// get frequency from radio
	try {
		return radio->getFreq();	// return current frequency
	} catch (const RigException& e) {
		fprintf(stderr, "Hamlib error %i: %s\n", e.errorno, e.message);
		return -1;
	}
	return -1; 	// well, something went wrong
}	// END OF 'get_radio_freq'

rmode_t get_radio_mode() {	// get mode from radio
	try {
		pbwidth_t bw;
		return radio->getMode(bw);	// return current mode
	} catch (const RigException& e) {
		fprintf(stderr, "Hamlib error %i: %s\n", e.errorno, e.message);
                return RIG_MODE_NONE;
	}
	return RIG_MODE_NONE;	// gotta return something
}	// END OF 'get_radio_mode'

bool set_radio_freq(freq_t f) {	// set radio to specified frequency
	try {
		radio->setFreq(Hz(f));
	} catch (const RigException& e) {
		fprintf(stderr, "Hamlib error %i: %s\n", e.errorno, e.message);
                return false;
	}
	return true;
}	// END OF 'set_radio_freq'

bool set_radio_mode(rmode_t m) {	// set radio to specified mode
	try {
                radio->setMode(m);
        } catch (const RigException& e) {
                fprintf(stderr, "Hamlib error %i: %s\n", e.errorno, e.message);
                return false;
        }
        return true;
}       // END OF 'set_radio_mode'
