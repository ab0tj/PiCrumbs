#include "beacon.h"

// GLOBAL VARS
extern float pos_lat;					// current latitude
extern char pos_lat_dir;				// latitude direction
extern float pos_lon;					// current longitude
extern char pos_lon_dir;				// current longitude direction
extern float gps_speed;					// speed from gps, in knots
extern short int gps_hdg;				// heading from gps
extern string temp_file;				// file to get 1-wire temp info from, blank to disable
extern bool temp_f;						// temp units: false for C, true for F
extern bool fh_debug;					// frequency hopping debug

// LOCAL VARS
string mycall;							// callsign we're operating under, excluding ssid
unsigned char myssid;					// ssid of this station (stored as a number, not ascii)
bool compress_pos;						// should we compress the aprs packet?
char symbol_table;						// which symbol table to use
char symbol_char;						// which symbol to use from the table
string beacon_comment;					// comment to send along with aprs packets
vector<aprspath> aprs_paths;			// APRS paths to try, in order of preference
unsigned int last_heard = 16;				// time since we heard a station on vhf
bool gpio_enable;						// can we use gpio pins

bool send_pos_report(int path = 0) {			// exactly what it sounds like
	time(&aprs_paths[path].lastused);			// update lastused time on path
	char* pos = new char[21];
	if (compress_pos) {		// build compressed position report, yes, byte by byte.
		pos[0] = '!';		// realtime position, no messaging
		pos[1] = symbol_table;
		float lat;
		float lon;
		float lat_min = modf(pos_lat/100, &lat);	// separate deg and min
		float lon_min = modf(pos_lon/100, &lon);
		lat += (lat_min/.6);	// convert min to deg and re-add it
		lon += (lon_min/.6);
		if (pos_lat_dir == 'S') lat = -lat;	// assign direction sign
		if (pos_lon_dir == 'W') lon = -lon;
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
		pos[10] = symbol_char;
		pos[11] = gps_hdg / 4 + 33;
		pos[12] = log(gps_speed+1)/log(1.08) + 33;
		pos[13] = 0x5F;			// set "T" byte
		pos[14] = 0x00;			// (null terminated string)
	} else {	// uncompressed packet
		sprintf(pos, "!%02.2f%c%c%03.2f%c%c", pos_lat, pos_lat_dir, symbol_table, pos_lon, pos_lon_dir, symbol_char);
	}
	stringstream buff;
	buff << pos;
	delete pos;		// memory leak fixed
	if (temp_file.compare("") != 0) {	// user specified a temp sensor is available
		float t = get_temp();
		if (t > -274 && t < 274) {		// don't bother sending the temp if we're violating the laws of physics or currently on fire.
			buff << t;
			if (temp_f) {
				buff << "F ";
			} else {
				buff << "C ";
			}
		}
	}
	buff << beacon_comment;
	if (aprs_paths[path].aprsis) {
		return send_aprsis_http(mycall.c_str(), myssid, PACKET_DEST, 0, aprs_paths[path].pathcalls, aprs_paths[path].pathssids, buff.str());
	} else {
		send_kiss_frame(aprs_paths[path].hf, mycall.c_str(), myssid, PACKET_DEST, 0, aprs_paths[path].pathcalls, aprs_paths[path].pathssids, buff.str());
		return true;
	}
}	// END OF 'send_pos_report'

int beacon() {		// try to send an APRS beacon
	if (last_heard < 10) return -1;		// hardcoded rate limiting
	int paths = aprs_paths.size();
	if (paths == 1) {		// no frequency hopping, just send a report
		if (gpio_enable && !check_gpio(0)) return -1;
		send_pos_report();
		return 0;
	} else {
		for (int i=0; i<paths; i++) {		// loop thru all paths
			if (fh_debug) printf("FH_DEBUG: Considering path%i.\n", i+1);
			if (time(NULL) - aprs_paths[i].lastused < aprs_paths[i].holdoff) continue;	// skip if we're not past the holdoff time
			if (aprs_paths[i].aprsis) {		// try immediately if this is an internet path
				if (send_pos_report(i)) {
					return i;
				} else continue;		// didn't work, try the next path
			}
			if (gpio_enable && !check_gpio(i)) continue;	// skip if gpio says no
			if (aprs_paths[i].sat.compare("") != 0) {	// if the user specified a sat for this path...
				if (is_visible(i)) {
					if (fh_debug) printf("FH_DEBUG: %s is visible.\n", aprs_paths[i].sat.c_str()); // sat is visible, keep going.
				} else {
					if (fh_debug) printf("FH_DEBUG: %s not visible.\n", aprs_paths[i].sat.c_str());
					continue;			// skip this path is this sat isn't visible
				}
			}
			if (!tune_radio(i)) continue;		// tune radio. skip if we can't tune this freq
			send_pos_report(i);					// passed all the tests. send a beacon.
			if (aprs_paths[i].hf) return i;		// don't bother listening for a digi on hf.
			sleep(6);
			if (last_heard > 15) {		// probably didn't get digi'd.
				if (!aprs_paths[i].retry) continue;		// move on to the next one if we aren't allowed to retry here
				if (fh_debug) printf("FH_DEBUG: Retrying.\n");
				if (!tune_radio(i)) continue;	// just in case user is messing with radio when we want to retry
				send_pos_report(i);				// try again
				sleep(6);
				if (last_heard < 15) return i;	// must have worked this time
			} else {
				return i;							// we did get digi'd
			}
		}	// if we made it this far, we didn't get a packet thru, loop to next path
		if (fh_debug) printf("FH_DEBUG: Giving up.\n");
		return -1;	// if we made it this far we are totally outta luck. return failure.
	}
} // END OF 'beacon'
