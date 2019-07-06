#include "gps.h"
#include "console.h"
#include "debug.h"
#include <unistd.h>
#include <pthread.h>
#include <mutex>
#include <libgpsmm.h>

namespace gps
{
	namespace
	{
		mutex currentPos_mutex;
		GpsPos currentPos;
		bool gotFix;
	}
	bool enabled;
	gpio::Led* led;

	void* gps_thread(void*) {
		gpsmm gps_rec("localhost", DEFAULT_GPSD_PORT);
		unique_lock<mutex> posLock(currentPos_mutex, defer_lock);

		if (gps_rec.stream(WATCH_ENABLE | WATCH_JSON) == NULL) {
			fprintf(stderr, "GPSD is not running.\n");
			return 0;
		}

		if (debug.verbose) printf("Waiting for GPS fix...");

		for (;;) {
			struct gps_data_t* newdata;

			if (!gps_rec.waiting(5000000)) {
				posLock.lock();
				currentPos.valid = false;
				posLock.unlock();
				if (led != NULL) led->setColor(gpio::Red);
				if (debug.gps) printf("GPS_DEBUG: GPSd timeout.\n");
				if (console::disp) console::conPrint("\x1B[4;6H\x1B[KGPSd timeout.");
				continue;
			}
			
			posLock.lock();
			if ((newdata = gps_rec.read()) == NULL) {	// can't get gps info
				currentPos.valid = false;
				if (led != NULL) led->setColor(gpio::Red);
				fprintf(stderr, "GPS read error.\n");
				posLock.unlock();
				continue;
			} else if (newdata->fix.mode > 1) {		// good fix
				if (!gotFix)
				{
					gotFix = true;
					if (debug.verbose) printf(" Ready!\n");
				}
				currentPos.valid = true;
				if (newdata->set & LATLON_SET) {
					currentPos.lat = newdata->fix.latitude;
					currentPos.lon = newdata->fix.longitude;
				}
				if (newdata->set & SPEED_SET) currentPos.speed = newdata->fix.speed * 2.23694;	// gpsd reports in m/s, covert to mph.
				if (newdata->set & TRACK_SET) currentPos.hdg = newdata->fix.track;
				if (newdata->set & ALTITUDE_SET) currentPos.alt = newdata->fix.altitude;
				
				if (debug.gps) printf("GPS_DEBUG: Lat:%f Lon:%f Alt:%i MPH:%.2f Hdg:%i Mode:%iD\n", currentPos.lat, currentPos.lon, currentPos.alt, currentPos.speed, currentPos.hdg, newdata->fix.mode);
				if (console::disp) dprintf(console::iface, "\x1B[4;6H\x1B[KLat:%f Lon:%f Alt:%i MPH:%.2f Hdg:%i Fix:%iD\n", currentPos.lat, currentPos.lon, currentPos.alt, currentPos.speed, currentPos.hdg, newdata->fix.mode);
			} else {
				currentPos.valid = false;	// no fix
				if (debug.gps) printf("GPS_DEBUG: No fix.\n");
				if (console::disp) console::conPrint("\x1B[4;6H\x1B[KNo Fix.");
			}
			bool valid = currentPos.valid;
			posLock.unlock();

			if (led != NULL)	// Blink the led based on fix status
			{
				if (led->isBicolor())
				{	// One red blink for invalid, one green blink for valid
					led->setColor(valid ? gpio::Green : gpio::Red);
					usleep(50000);
					led->setColor(gpio::LedOff);
				}
				else
				{	// Invalid = off with blink to show data received, valid = the opposite
					led->setColor(valid ? gpio::LedOff : gpio::Green);
					usleep(50000);
					led->setColor(valid ? gpio::Green : gpio::LedOff);
				}
			}
		}
	}

	GpsPos getPos()
	{
		lock_guard<mutex> guard(currentPos_mutex);
		return currentPos;
	}
}