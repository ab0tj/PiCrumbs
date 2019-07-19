#include <string>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <unistd.h>
#include "hamlib.h"
#include "gpio.h"
#include "http.h"
#include "predict.h"
#include "tnc.h"
#include "console.h"
#include "version.h"
#include "beacon.h"
#include "psk.h"
#include "gps.h"
#include "debug.h"

namespace beacon
{
	string mycall;						// callsign we're operating under, excluding ssid
	unsigned char myssid;				// ssid of this station (stored as a number, not ascii)
	bool compress;
	bool send_course;
	bool send_speed;
	bool send_alt;
	char symbol_table;					// which symbol table to use
	char symbol_char;					// which symbol to use from the table
	string comment;						// comment to send along with aprs packets
	string status;
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
	unsigned int status_rate;
	int status_path;
	gpio::Led* led;						// LED to display beacon status
	sensor::Adc* adcs[8];
	sensor::Temp* tempSensor;

	bool send_packet(aprspath& path, string payload)
	{
		switch (path.proto) {	// choose the appropriate way to send the beacon
			case VHF_AX25:
			case HF_AX25:
				send_kiss_frame((path.proto == HF_AX25), mycall.c_str(), myssid, PACKET_DEST, 0, path.pathcalls, path.pathssids, payload);
				return true;

			case APRS_IS:
				return send_aprsis_http(mycall.c_str(), myssid, PACKET_DEST, 0, path.pathcalls, path.pathssids, payload);

			case PSK63:
				if (pskPttPin != NULL) {
					send_psk_aprs(path.psk_freq, path.psk_vol, pskPttPin, mycall.c_str(), myssid, PACKET_DEST, 0, payload.c_str());
					return true;
				}
				return false;	// can't send psk without gpio (yet)

			case PSKAndAX25:
				if (!path.last_psk && pskPttPin != NULL) {	// send psk
					send_psk_aprs(path.psk_freq, path.psk_vol, pskPttPin, mycall.c_str(), myssid, PACKET_DEST, 0, payload.c_str());
					path.last_psk = true;
				} else {	// send 300bd
					send_kiss_frame(true, mycall.c_str(), myssid, PACKET_DEST, 0, path.pathcalls, path.pathssids, payload);
					path.last_psk = false;
				}
				return true;

			default:
				return false;
		}
	}

	float fround(float f, uint digits)
	{
		if (digits == 0) return round(f);
		return float(int(f * pow(10, digits) + 0.5)) / pow(10, digits);
	}

	string parseComment(string text)
	{
		int textSz = text.length();
		stringstream buff;

		for (int i=0; i<textSz; i++)
		{
			if (text[i] == '|') continue;	// APRS spec says comments cannot contain ~ or |
			if (text[i] == '~')
			{
				int p = text[i+2] - '0';
				switch (text[i+1])
				{
					case 'a':   // Scaled ADC value
						if (adcs[p] != NULL) buff << fround(adcs[p]->read(1), 2);
						i += 2;
						break;

					case 'r':   // Raw ADC value
						if (adcs[p] != NULL) buff << adcs[p]->read(0);
						i += 2;
						break;

					case 't':   // Temperature value
						if (tempSensor != NULL) buff << fround(tempSensor->read(), tempSensor->precision) << tempSensor->get_unit();
						i += 1;
						break;

					case 'z':   // Timestamp
						char zulu[8];
						time_t rawtime;
						struct tm* timeinfo;
						time(&rawtime);
						timeinfo = gmtime(&rawtime);
						strftime(zulu, 8, "%d%H%Mz", timeinfo);
						buff << zulu;
						i += 1;
						break;

					default:
						break;
				}
			}
			else buff << text[i];
		}
		return buff.str();
	}

	bool send_pos_report(aprspath& path = aprs_paths[0]) {			// exactly what it sounds like
		stringstream buff;
		gps::PosStruct gps = gps::getPos();

		time(&path.lastused);			// update lastused time on path
		path.attempt++;					// update stats
		
		float speed = gps.speed * 1.94384;	// convert m/s to knots
		uint alt = gps.alt * 3.28084;		// convert meters to feet
		char* pos = new char[21];
		if (compress) {			// build compressed position report as an array of bytes
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

			if (!send_course && !send_speed && !send_alt)
			{
				pos[11] = pos[12] = pos[13] = ' ';
			}
			else if (send_course || send_speed)
			{
				pos[11] = pos[12] = 33;
				if (send_course) pos[11] += gps.hdg / 4;
				if (send_speed) pos[12] += log(speed+1)/log(1.08);
				pos[13] = 0x47;			// set "T" byte
			}
			else	// send altitude
			{
				int x = log(alt) / log(1.002);
				pos[11] = x / 91 + 33;
				pos[12] = x % 91 + 33;
				pos[13] = 0x57;
			}

			pos[14] = 0x00;			// (null terminated string)
			buff << pos;

		} else {	// uncompressed packet
			char pos_lat_dir = 'N';
			char pos_lon_dir = 'E';
			if (gps.lat < 0) pos_lat_dir = 'S';
			if (gps.lon < 0) pos_lon_dir = 'W';

			sprintf(pos, "!%05.2f%c%c%06.2f%c%c", abs(int(gps.lat)*100 + (gps.lat-int(gps.lat))*60) , pos_lat_dir, symbol_table
								, abs(int(gps.lon)*100 + (gps.lon-int(gps.lon))*60) , pos_lon_dir, symbol_char);
			buff << pos;

			if (send_course || send_speed)
			{
				if (send_course && send_speed) sprintf(pos, "%03d/%03d", gps.hdg, (int)speed);
				else if (send_course && !send_speed) sprintf(pos, "%03d/...", gps.hdg);
				else sprintf(pos, ".../%03d", (int)speed);
				buff << pos;
			}
			if (send_alt)
			{
				sprintf(pos, "/A=%06d", alt);
				buff << pos;
			}
		}
		delete[] pos;

		if (path.usePathComment)
		{
			buff << parseComment(path.comment);
		}
		else if (comment.compare("") != 0)
		{
			buff << parseComment(comment);
		}
		
		return send_packet(path, buff.str());
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

	int send(BeaconType type) {
		freq_t radio_freq;
		rmode_t radio_mode;
		int path;

		if (radio_retune) {
			radio_freq = get_radio_freq();			// save radio frequency
			radio_mode = get_radio_mode();			// save radio mode
			if (debug.verbose) printf("FH_DEBUG: Current radio frequency is %.0f %s\n", radio_freq, rig_strrmode(radio_mode));
		}

		switch (type)
		{
			case Position:
				// blink the led while we try to send a packet
				led->set(gpio::LedOff, gpio::Blink, led->isBicolor() ? led->getColor() : gpio::Green);
				path = path_select_beacon();	// send a beacon and do some housekeeping afterward
				if (path != -1)
				{
					aprs_paths[path].success++;	// update stats
					if (led != NULL) led->set(gpio::Green);
				}
				else if (led != NULL) led->set(gpio::Red);

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
				
				if (console::disp) console::show_pathstats(true);
				break;

			case Status:
				path = status_path;
				if (aprs_paths[path].enablePin != NULL && !aprs_paths[path].enablePin->read())
				{
					if (debug.fh) printf("FH_DEBUG: Status path disabled via GPIO\n");
					path = -1;
					break;
				}
				
				if (tune_radio(aprs_paths[path].freq, aprs_paths[path].mode))
				{
					send_packet(aprs_paths[status_path], ">" + parseComment(status));
					sleep(5);	// give time for the packet to transmit
				}
				else path = -1;
				break;
		}

		if (radio_retune) {
			tune_radio(radio_freq, radio_mode);
		}
		else if (path != status_path)
		{
			tune_radio(aprs_paths[status_path].freq, aprs_paths[status_path].mode);
		}

		return path;
	}
}
