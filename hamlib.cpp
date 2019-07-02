#include <unistd.h>
#include "debug.h"
#include "hamlib.h"

extern BeaconStruct beacon;
HamlibStruct hamlib;

bool tune_radio(aprspath* path) {		// use hamlib to tune the radio to the freq and mode of this path
	if (!hamlib.enabled) return true;	// don't try to do tuning stuff if we can't

	try {
		if (hamlib.radio->getFreq() == path->freq) return true; // skip if already set
		if (debug.hl) printf("HL_DEBUG: Tuning radio to %.0f\n", path->freq);
		hamlib.radio->setFreq(Hz(path->freq));
		hamlib.radio->setMode(path->mode);
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
		return hamlib.radio->getFreq();	// return current frequency
	} catch (const RigException& e) {
		fprintf(stderr, "Hamlib error %i: %s\n", e.errorno, e.message);
		return -1;
	}
	return -1; 	// well, something went wrong
}	// END OF 'get_radio_freq'

rmode_t get_radio_mode() {	// get mode from radio
	try {
		pbwidth_t bw;
		return hamlib.radio->getMode(bw);	// return current mode
	} catch (const RigException& e) {
		fprintf(stderr, "Hamlib error %i: %s\n", e.errorno, e.message);
                return RIG_MODE_NONE;
	}
	return RIG_MODE_NONE;	// gotta return something
}	// END OF 'get_radio_mode'

bool set_radio_freq(freq_t f) {	// set radio to specified frequency
	try {
		hamlib.radio->setFreq(Hz(f));
	} catch (const RigException& e) {
		fprintf(stderr, "Hamlib error %i: %s\n", e.errorno, e.message);
                return false;
	}
	return true;
}	// END OF 'set_radio_freq'

bool set_radio_mode(rmode_t m) {	// set radio to specified mode
	try {
                hamlib.radio->setMode(m);
        } catch (const RigException& e) {
                fprintf(stderr, "Hamlib error %i: %s\n", e.errorno, e.message);
                return false;
        }
        return true;
}       // END OF 'set_radio_mode'

void hamlib_close()
{
	hamlib.radio->close();
}
