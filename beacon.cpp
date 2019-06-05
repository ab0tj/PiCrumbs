#include "beacon.h"
#include <string>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <unistd.h>
#include "hamlib.h"
#include "pi.h"
#include "http.h"
#include "predict.h"
#include "tnc.h"
#include "console.h"
#include "version.h"
#include "psk.h"
#include "gps.h"
#include "debug.h"

BeaconStruct beacon;
extern PiStruct pi;
extern DebugStruct debug;
extern ConsoleStruct console;

bool send_pos_report(aprspath* path = &beacon.aprs_paths[0]) {			// exactly what it sounds like
	stringstream buff;
	GpsPos gps = gps::getPos();

	time(&path->lastused);			// update lastused time on path
	path->attempt++;					// update stats
	
	char* pos = new char[21];
	if (beacon.compress_pos) {		// build compressed position report as an array of bytes
		float speed = gps.speed * 0.868976;				// convert mph to knots
		pos[0] = '!';		// realtime position, no messaging
		pos[1] = beacon.symbol_table;
		float lat = gps.lat;	// grab a copy of our location so we can do math on it
		float lon = gps.lon;
		lat = 380926 * (90 - lat);		// formula from aprs spec
		lon = 190463 * (180 + lon);
		pos[2] = (int)lat / 753571 + 33;	// lat/91^3+33
		lat = (int)lat % 753571;			// remainder
		pos[3] = (int)lat / 8281 + 33;		// remainder/91^2+33
		lat = (int)lat % 8281;				// remainder
		pos[4] = (int)lat / 91 + 33;		// remainder/91^1+33
		pos[5] = (int)lat % 91 + 33;		// remainder + 33
		pos[6] = (int)lon / 753571 + 33;    // do the same for long.
		lon = (int)lon % 753571;
		pos[7] = (int)lon / 8281 + 33;
		lon = (int)lon % 8281;
		pos[8] = (int)lon / 91 + 33;
		pos[9] = (int)lon % 91 + 33;
		pos[10] = beacon.symbol_char;
		pos[11] = gps.hdg / 4 + 33;
		pos[12] = log(speed+1)/log(1.08) + 33;
		pos[13] = 0x5F;			// set "T" byte
		pos[14] = 0x00;			// (null terminated string)
	} else {	// uncompressed packet
		char pos_lat_dir = 'N';
		char pos_lon_dir = 'E';
		if (gps.lat < 0) pos_lat_dir = 'S';
		if (gps.lon < 0) pos_lon_dir = 'W';

		sprintf(pos, "!%05.2f%c%c%06.2f%c%c", abs(int(gps.lat)*100 + (gps.lat-int(gps.lat))*60) , pos_lat_dir, beacon.symbol_table
						    , abs(int(gps.lon)*100 + (gps.lon-int(gps.lon))*60) , pos_lon_dir, beacon.symbol_char);
	}
	buff << pos;
	delete[] pos;

	if (pi.temp_file.compare("") != 0) {	// user specified a temp sensor is available
		float t = get_temp();
		if (t > -274 && t < 274) {		// don't bother sending the temp if we're violating the laws of physics or currently on fire.
			buff << t;
			if (pi.temp_f) {
				buff << "F ";
			} else {
				buff << "C ";
			}
		}
	}

	if (path->usePathComment)
	{
		buff << path->comment;
	}
	else
	{
		buff << beacon.comment;
	}
	
	switch (path->proto) {	// choose the appropriate way to send the beacon
		case 0:	// 1200 baud
		case 1:	// 300 baud
			send_kiss_frame((path->proto == 1), beacon.mycall.c_str(), beacon.myssid, PACKET_DEST, 0, path->pathcalls, path->pathssids, buff.str());
			return true;
		case 2:	// aprs-is path
			return send_aprsis_http(beacon.mycall.c_str(), beacon.myssid, PACKET_DEST, 0, path->pathcalls, path->pathssids, buff.str());
		case 3:	// psk63 path
			if (pi.gpio_enable) {
				send_psk_aprs(path->psk_freq, path->psk_vol, pi.gpio_psk_ptt, beacon.mycall.c_str(), beacon.myssid, PACKET_DEST, 0, buff.str().c_str());
				return true;
			}
			return false;	// can't send psk without gpio (yet)
		case 4: // alternate 300bd/psk
			if (!path->last_psk && pi.gpio_enable) {	// send psk
				send_psk_aprs(path->psk_freq, path->psk_vol, pi.gpio_psk_ptt, beacon.mycall.c_str(), beacon.myssid, PACKET_DEST, 0, buff.str().c_str());
				path->last_psk = true;
			} else {	// send 300bd
				send_kiss_frame(true, beacon.mycall.c_str(), beacon.myssid, PACKET_DEST, 0, path->pathcalls, path->pathssids, buff.str());
				path->last_psk = false;
			}
			return true;
	}
	
	return false;
}	// END OF 'send_pos_report'

bool wait_for_digi() {
	int timeout = 6;

	while (beacon.last_heard > 15) {
		sleep(1);
		if (--timeout <= 0) return false;
	}

	return true;	// last heard is less than 15, must have been digi'd.
}	// END OF 'wait_for_digi'

int path_select_beacon() {		// try to send an APRS beacon
	if (beacon.last_heard < 10) return -1;		// hardcoded rate limiting
	int paths = beacon.aprs_paths.size();
	if (paths == 1) {		// no frequency hopping, just send a report
		if (pi.gpio_enable && !check_gpio(0)) return -1;
		send_pos_report();
		return 0;
	} else {
		for (int i=0; i < paths; i++) {		// loop thru all paths
			aprspath* path = &beacon.aprs_paths[i];
			if (debug.fh) printf("FH_DEBUG: Trying %s\n", path->name.c_str());
			if ((unsigned int)(time(NULL) - path->lastused) < path->holdoff) continue;	// skip if we're not past the holdoff time
			if (path->proto == 2) {		// try immediately if this is an internet path
				if (send_pos_report(path)) {
					return i;
				} else continue;		// didn't work, try the next path
			}
			if (pi.gpio_enable && !check_gpio(i)) continue;	// skip if gpio says no
			if (path->sat.compare("") != 0) {	// if the user specified a sat for this path...
				if (is_visible(path->sat, path->min_ele)) {
					if (debug.fh) printf("FH_DEBUG: %s is visible\n", path->sat.c_str()); // sat is visible, keep going.
				} else {
					if (debug.fh) printf("FH_DEBUG: %s not visible\n", path->sat.c_str());
					continue;			// skip this path is this sat isn't visible
				}
			}
			if (!tune_radio(path)) continue;		// tune radio. skip if we can't tune this freq
			send_pos_report(path);					// passed all the tests. send a beacon.
			if (path->proto == 1) sleep(10);	// give hf packet time to transmit
			if (path->proto != 0) return i;		// don't bother listening for a digi if this isn't vhf.

			if (!wait_for_digi()) {		// probably didn't get digi'd.
				if (!path->retry) continue;		// move on to the next one if we aren't allowed to retry here
				if (debug.fh) printf("FH_DEBUG: Retrying\n");
				if (!tune_radio(path)) continue;	// just in case user is messing with radio when we want to retry
				send_pos_report(path);				// try again
				if (wait_for_digi()) return i;	// must have worked this time
			} else {
				return i;							// we did get digi'd
			}
		}	// if we made it this far, we didn't get a packet thru, loop to next path

		return -1;	// if we made it this far we are totally outta luck. return failure.
	}
} // END OF 'path_select_beacon'

int sendBeacon() {
	freq_t radio_freq;
	rmode_t radio_mode;

	if (beacon.radio_retune) {
		radio_freq = get_radio_freq();			// save radio frequency
		radio_mode = get_radio_mode();			// save radio mode
	}

	int path = path_select_beacon();			// send a beacon and do some housekeeping afterward
	
	if (beacon.radio_retune) {
		set_radio_freq(radio_freq);				// return radio to previous frequency
		set_radio_mode(radio_mode);				// return radio to previous mode
	}
	else if (path != 0) tune_radio(&beacon.aprs_paths[0]);

	if (path != -1) beacon.aprs_paths[path].success++;	// update stats

	if (debug.fh)
	{
		if (path == -1)
		{
			printf("FH_DEBUG: Path select returned failure\n");
		}
		else
		{
			printf("FH_DEBUG: Success via %s\n", beacon.aprs_paths[path].name.c_str());
		}
		
	}
	
	if (console.disp) show_pathstats(true);
	return path;
}
