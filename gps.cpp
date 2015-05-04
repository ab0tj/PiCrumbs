#include "gps.h"
#include <sys/time.h>
#include <string>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>

// GLOBAL VARS
extern bool verbose;
extern bool gps_debug;

// LOCAL VARS
int gps_iface;						// gps serial port fd
bool beacon_ok;						// should we be sending beacons?
float pos_lat;						// current latitude
char pos_lat_dir;					// latitude direction
float pos_lon;						// current longitude
char pos_lon_dir;					// current longitude direction
unsigned short int pos_alt;			// current altitude in meters
struct tm * gps_time = new tm;				// last time received from the gps (if enabled)
bool gps_tm_sync;					// should we sync the system time to gps time?
float gps_speed;					// speed from gps, in knots
short int gps_hdg;					// heading from gps

void* gps_thread(void*) {		// thread to listen to the incoming NMEA stream and update our position and time
	unsigned char clock_sync = 60;	// sync local clock as soon as we have a gps fix
	string buff = "";
	char * data = new char[1];

	while (true) {
		read(gps_iface, data, 1);
		if (data[0] == '\n') {			// NMEA data is terminated with a newline.
			if (buff.length() > 6) {	// but let's not bother if this was an incomplete line
				if (buff.compare(0, 6, "$GPRMC") == 0) {	// GPRMC has most of the info we care about
					string params[12];
					int current = 7;
					int next = 0;
					for (int a=0; a<12; a++) {		// here we'll split the fields of the GPRMC sentence
						next = buff.find_first_of(',', current);
						params[a] = buff.substr(current, next - current);
						current = next + 1;
					}
					if (params[1].compare("A") == 0) {	// GPS says data is valid.
						gps_time->tm_hour = atoi(params[0].substr(0,2).c_str());
						gps_time->tm_min = atoi(params[0].substr(2,2).c_str());
						gps_time->tm_sec = atoi(params[0].substr(4,2).c_str());
						gps_time->tm_mday = atoi(params[8].substr(0,2).c_str());
						gps_time->tm_mon = atoi(params[8].substr(2,2).c_str()) - 1;		// tm_mon is 0-11
						gps_time->tm_year = atoi(params[8].substr(4,2).c_str()) + 100; 	// tm_year is "years since 1900", and apparently GPRMC isn't Y2K compliant.
						pos_lat = atof(params[2].c_str());
						pos_lat_dir = params[3][0];
						pos_lon = atof(params[4].c_str());
						pos_lon_dir = params[5][0];
						gps_speed = atof(params[6].c_str());
						gps_hdg = atoi(params[7].c_str());
						if (gps_tm_sync && clock_sync++ >= 60) {
							timeval tv;
							tv.tv_sec = mktime(gps_time);
							long int now = time(NULL);
							if (tv.tv_sec != now) {
								if (verbose) printf("Bumping system clock by %li seconds.\n", tv.tv_sec - now);
								settimeofday(&tv, NULL);		// set system clock to match gps
							}
							clock_sync = 0;
						}
						if (gps_debug) {
							mktime(gps_time);	// update tm_wday
							printf("GPS_DEBUG: Lat:%f%c Long:%f%c MPH:%.2f Hdg:%i Time:%s", pos_lat, pos_lat_dir, pos_lon, pos_lon_dir, gps_speed * 1.15078, gps_hdg, asctime(gps_time));
						}
						beacon_ok = true;
					} else {
						beacon_ok = false;
						if (gps_debug) printf("GPS_DEBUG: no fix or invalid data.\n");
					}
				}
				if (buff.compare(0, 6, "$GPGGA") == 0) {	// GPGGA has the altitude
					string params[15];
					int current = 7;
					int next = 0;
					for (int a=0; a<15; a++) {		// here we'll split the fields of the GPGGA sentence
						next = buff.find_first_of(',', current);
						params[a] = buff.substr(current, next - current);
						current = next + 1;
					}
					if (params[5].compare("0") != 0) {	// this will be 0 if invalid
						pos_alt = atoi(params[8].c_str());
						if (gps_debug) printf("GPS_DEBUG: Alt:%i\n", pos_alt);
					}
				}
			}
			buff = "";		// clear the buffer
		} else {
			buff.append(1, data[0]);	// this wasn't a newline so just add it to the buffer.
		}
	}
	return 0;
} // END OF 'gps_thread'
