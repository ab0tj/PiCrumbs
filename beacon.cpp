#include "beacon.h"
#include <string>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include "hamlib.h"
#include "gpio.h"
#include "http.h"
#include "predict.h"
#include "tnc.h"
#include "console.h"
#include "version.h"
#include "psk.h"
#include "gps.h"
#include "debug.h"

extern ConsoleStruct console;

namespace beacon
{
	string mycall;						// callsign we're operating under, excluding ssid
	unsigned char myssid;				// ssid of this station (stored as a number, not ascii)
	bool compress_pos;					// should we compress the aprs packet?
	char symbol_table;					// which symbol table to use
	char symbol_char;					// which symbol to use from the table
	string comment;						// comment to send along with aprs packets
	vector<aprspath> aprs_paths;		// APRS paths to try, in order of preference
	unsigned int last_heard;			// time since we heard our call on vhf
	bool radio_retune;					// should we retune the radio after beaconing?
	unsigned short int sb_low_speed; 	// SmartBeaconing low threshold, in mph
	unsigned int sb_low_rate;			// SmartBeaconing low rate
	unsigned short int sb_high_speed; 	// SmartBeaconing high threshold, in mph
	unsigned int sb_high_rate;			// SmartBeaconing high rate
	unsigned short int sb_turn_min;		// SmartBeaconing turn minimum (deg)
	unsigned short int sb_turn_time; 	// SmartBeaconing turn time (minimum)
	unsigned short int sb_turn_slope; 	// SmartBeaconing turn slope
	unsigned int static_rate;			// how often (in seconds) to send a beacon if not using gps, set to 0 for SmartBeaconing
    string temp_file;					// file to get temperature info from, blank to disable
    bool temp_f;						// temp units: false for C, true for F
	string adc_file;					// file to get ADC value from, blank to disable
	float adc_scale;					// ADC scaling value
	gpio::Led* led;						// LED to display beacon status
	
	int get_temp();
	float get_voltage();

	bool send_pos_report(aprspath& path = aprs_paths[0]) {			// exactly what it sounds like
		stringstream buff;
		GpsPos gps = gps::getPos();

		time(&path.lastused);			// update lastused time on path
		path.attempt++;					// update stats
		
		char* pos = new char[21];
		if (compress_pos) {		// build compressed position report as an array of bytes
			float speed = gps.speed * 0.868976;				// convert mph to knots
			pos[0] = '!';		// realtime position, no messaging
			pos[1] = symbol_table;
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
			pos[10] = symbol_char;
			pos[11] = gps.hdg / 4 + 33;
			pos[12] = log(speed+1)/log(1.08) + 33;
			pos[13] = 0x5F;			// set "T" byte
			pos[14] = 0x00;			// (null terminated string)
		} else {	// uncompressed packet
			char pos_lat_dir = 'N';
			char pos_lon_dir = 'E';
			if (gps.lat < 0) pos_lat_dir = 'S';
			if (gps.lon < 0) pos_lon_dir = 'W';

			sprintf(pos, "!%05.2f%c%c%06.2f%c%c", abs(int(gps.lat)*100 + (gps.lat-int(gps.lat))*60) , pos_lat_dir, symbol_table
								, abs(int(gps.lon)*100 + (gps.lon-int(gps.lon))*60) , pos_lon_dir, symbol_char);
		}
		buff << pos;
		delete[] pos;

		if (temp_file.compare("") != 0) {	// user specified a temp sensor is available
			int t = get_temp();
			if (t > -274 && t < 274) {		// don't bother sending the temp if we're violating the laws of physics or currently on fire.
				buff << t;
				if (temp_f) {
					buff << "F ";
				} else {
					buff << "C ";
				}
			}
		}

		if (adc_file.compare("") != 0)
		{
			float volts = get_voltage();
			if (volts != -1) buff << volts << "V ";
		}

		if (path.usePathComment)
		{
			buff << path.comment;
		}
		else
		{
			buff << comment;
		}
		
		switch (path.proto) {	// choose the appropriate way to send the beacon
			case VHF_AX25:
			case HF_AX25:
				send_kiss_frame((path.proto == HF_AX25), mycall.c_str(), myssid, PACKET_DEST, 0, path.pathcalls, path.pathssids, buff.str());
				return true;
			case APRS_IS:
				return send_aprsis_http(mycall.c_str(), myssid, PACKET_DEST, 0, path.pathcalls, path.pathssids, buff.str());
			case PSK63:
				if (pskPttPin != NULL) {
					send_psk_aprs(path.psk_freq, path.psk_vol, pskPttPin, mycall.c_str(), myssid, PACKET_DEST, 0, buff.str().c_str());
					return true;
				}
				return false;	// can't send psk without gpio (yet)
			case PSKAndAX25:
				if (!path.last_psk && pskPttPin != NULL) {	// send psk
					send_psk_aprs(path.psk_freq, path.psk_vol, pskPttPin, mycall.c_str(), myssid, PACKET_DEST, 0, buff.str().c_str());
					path.last_psk = true;
				} else {	// send 300bd
					send_kiss_frame(true, mycall.c_str(), myssid, PACKET_DEST, 0, path.pathcalls, path.pathssids, buff.str());
					path.last_psk = false;
				}
				return true;
		}
		
		return false;
	}	// END OF 'send_pos_report'

	bool wait_for_digi() {
		int timeout = 6;

		while (last_heard > 15) {
			sleep(1);
			if (--timeout <= 0) return false;
		}

		return true;	// last heard is less than 15, must have been digi'd.
	}	// END OF 'wait_for_digi'

	int path_select_beacon() {		// try to send an APRS beacon
		if (last_heard < 10) return -1;		// hardcoded rate limiting

		for (uint i=0; i < aprs_paths.size(); i++) {		// loop thru all paths
			aprspath& path = aprs_paths[i];
			if (debug.fh) printf("FH_DEBUG: Trying %s\n", path.name.c_str());
			if ((unsigned int)(time(NULL) - path.lastused) < path.holdoff) continue;	// skip if we're not past the holdoff time

			if (path.enablePin != NULL && !path.enablePin->read())	// skip if gpio says no
			{
				if (debug.fh) printf("FH_DEBUG: Path disabled via GPIO.\n");
				continue;
			}

			if (path.proto == APRS_IS) {		// try immediately if this is an internet path
				if (send_pos_report(path)) {
					return i;
				} else continue;		// didn't work, try the next path
			}

			if (path.sat.compare("") != 0) {	// if the user specified a sat for this path...
				if (is_visible(path.sat, path.min_ele)) {
					if (debug.fh) printf("FH_DEBUG: %s is visible\n", path.sat.c_str()); // sat is visible, keep going.
				} else {
					if (debug.fh) printf("FH_DEBUG: %s not visible\n", path.sat.c_str());
					continue;			// skip this path is this sat isn't visible
				}
			}

			if (!tune_radio(path.freq, path.mode)) continue;		// tune radio. skip if we can't tune this freq
			send_pos_report(path);					// passed all the tests. send a beacon.
			if (path.proto == HF_AX25) sleep(10);	// give hf packet time to transmit
			if (path.proto != VHF_AX25) return i;		// don't bother listening for a digi if this isn't vhf.

			if (!wait_for_digi()) {		// probably didn't get digi'd.
				if (!path.retry) continue;		// move on to the next one if we aren't allowed to retry here
				if (debug.fh) printf("FH_DEBUG: Retrying\n");
				if (!tune_radio(path.freq, path.mode)) continue;	// just in case user is messing with radio when we want to retry
				send_pos_report(path);				// try again
				if (wait_for_digi()) return i;	// must have worked this time
			} else {
				return i;							// we did get digi'd
			}
		}	// if we made it this far, we didn't get a packet thru, loop to next path

		return -1;	// if we made it this far we are totally outta luck. return failure.
	} // END OF 'path_select_beacon'

	int send() {
		freq_t radio_freq;
		rmode_t radio_mode;

		if (radio_retune) {
			radio_freq = get_radio_freq();			// save radio frequency
			radio_mode = get_radio_mode();			// save radio mode
			if (debug.verbose) printf("FH_DEBUG: Current radio frequency is %.0f %s\n", radio_freq, rig_strrmode(radio_mode));
		}

		int path = path_select_beacon();			// send a beacon and do some housekeeping afterward
		
		if (radio_retune) {
			set_radio_freq(radio_freq);				// return radio to previous frequency
			set_radio_mode(radio_mode);				// return radio to previous mode
		}
		else if (path != 0) tune_radio(aprs_paths[0].freq, aprs_paths[0].mode);

		if (path != -1)
		{
			aprs_paths[path].success++;	// update stats
			if (led != NULL) led->setColor(gpio::Green);
		}
		else if (led != NULL) led->setColor(gpio::Red);

		if (debug.fh)
		{
			if (path == -1)
			{
				printf("FH_DEBUG: Path select returned failure\n");
			}
			else
			{
				printf("FH_DEBUG: Success via %s\n", aprs_paths[path].name.c_str());
			}
			
		}
		
		if (console.disp) show_pathstats(true);
		return path;
	}

	int get_w1_temp() {	// get temperature from one-wire sensor
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
	} // END OF 'get_w1_temp'

	int get_temp()	// get temperature from hwmon formatted file
	{
		ifstream tempfile;
		string line;
		tempfile.open(temp_file.c_str());
		if (!tempfile.is_open()) 
		{
			fprintf(stderr, "Error opening temperature file: %s\n", strerror(errno));
			return -65535;		// return absurdly low value so we know it's invalid.
		}
		getline(tempfile, line);
		int temp = atoi(line.c_str());
		if (temp_f) temp = 1.8*temp+32000;
		temp /= 1000;
		tempfile.close();
		return temp;
	}

	float get_voltage()	// get voltage from adc
	{
		ifstream adcfile;
		string line;
		adcfile.open(adc_file.c_str());
		if (!adcfile.is_open())
		{
			fprintf(stderr, "Error opening ADC file: %s\n", strerror(errno));
			return -1;		// can't have negative voltage...
		}
		getline(adcfile, line);
		float volts = atoi(line.c_str());
		volts *= adc_scale;
		adcfile.close();
		return volts;
	}
}
