#include "hamlib.h"

// GLOBAL VARS
extern vector<aprspath> aprs_paths;

// LOCAL VARS
Rig* radio;								// radio control interface reference

bool tune_radio(int path) {		// use hamlib to tune the radio to the freq and mode of this path
	try {
		if (radio->getFreq() == aprs_paths[path].freq) return true; // skip if already set
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