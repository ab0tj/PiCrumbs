#include "gps.h"
#include "console.h"
#include <pthread.h>
#include <libgpsmm.h>

// GLOBAL VARS
extern bool verbose;
extern bool gps_debug;
extern bool console_disp;				// print smartbeaconing params to console
extern int console_iface;				// console serial port fd

// LOCAL VARS
int gps_iface;						// gps serial port fd
bool gps_valid;						// should we be sending beacons?
float pos_lat;						// current latitude
float pos_lon;						// current longitude
unsigned short int pos_alt;			// current altitude in meters
float gps_speed;					// speed from gps, in mph
short int gps_hdg;					// heading from gps

void* gps_thread(void*) {
	gpsmm gps_rec("localhost", DEFAULT_GPSD_PORT);

    if (gps_rec.stream(WATCH_ENABLE | WATCH_JSON) == NULL) {
        fprintf(stderr, "GPSD is not running.\n");
        return 0;
    }

	if (verbose) printf("Waiting for GPS fix.\n");

    for (;;) {
		struct gps_data_t* newdata;

		if (!gps_rec.waiting(5000000)) {
			gps_valid = false;
			if (gps_debug) printf("GPS_DEBUG: GPSd timeout.\n");
			if (console_disp) console_print("\x1B[4;6H\x1B[KGPSd timeout.");
			continue;
		}
		
		if ((newdata = gps_rec.read()) == NULL) {	// can't get gps info
			gps_valid = false;
			fprintf(stderr, "GPS read error.\n");
		} else if (newdata->fix.mode > 1) {		// good fix
			gps_valid = true;
			if (newdata->set & LATLON_SET) {
				pos_lat = newdata->fix.latitude;
				pos_lon = newdata->fix.longitude;
			}
			if (newdata->set & SPEED_SET) gps_speed = newdata->fix.speed * 2.23694;	// gpsd reports in m/s, covert to mph.
			if (newdata->set & TRACK_SET) gps_hdg = newdata->fix.track;
			if (newdata->set & ALTITUDE_SET) pos_alt = newdata->fix.altitude;
			
			if (gps_debug) printf("GPS_DEBUG: Lat:%f Lon:%f Alt:%i MPH:%.2f Hdg:%i Mode:%iD\n", pos_lat, pos_lon, pos_alt, gps_speed, gps_hdg, newdata->fix.mode);
			if (console_disp) dprintf(console_iface, "\x1B[4;6H\x1B[KLat:%f Lon:%f Alt:%i MPH:%.2f Hdg:%i Mode:%iD\n", pos_lat, pos_lon, pos_alt, gps_speed, gps_hdg, newdata->fix.mode);
		} else {
			gps_valid = false;	// no fix
			if (gps_debug) printf("GPS_DEBUG: No fix.\n");
			if (console_disp) console_print("\x1B[4;6H\x1B[KNo Fix.");
		}
		
    }
}