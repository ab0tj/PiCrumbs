#include "main.h"
#include <pthread.h>
#include <cstdio>
#include <signal.h>
#include <unistd.h>
#include <cstdlib>
#include <curl/curl.h>
#include <sys/timerfd.h>
#include "beacon.h"
#include "gps.h"
#include "tnc.h"
#include "console.h"
#include "init.h"
#include "debug.h"
#include "hamlib.h"
#include "psk.h"

extern ConsoleStruct console;
extern TncStruct tnc;

void cleanup(int sign) {	// clean up after catching ctrl-c
	if (debug.verbose) printf("\nCleaning up.\n");
	delete beacon::led;
	delete gps::led;
	delete pskPttPin;
	close(tnc.vhf_iface);
	close(tnc.hf_iface);
	close(console.iface);
	hamlib_close();
	curl_global_cleanup();
	exit (EXIT_SUCCESS);
} // END OF 'cleanup'

int main(int argc, char* argv[]) {
	signal(SIGINT,&cleanup);	// catch ctrl-c

	init(argc, argv);	// get everything ready to go

	pthread_t tnc_t;
	pthread_create(&tnc_t, NULL, &tnc_thread, NULL);

	if (gps::enabled) {
		pthread_t gps_t;
		pthread_create(&gps_t, NULL, &gps::gps_thread, NULL);	// start the gps interface thread if the gps interface was opened
	}

	if (console.iface > 0) {	// start the console interface if the port was opened
		pthread_t console_t;
		pthread_create(&console_t, NULL, &console_thread, NULL);
	}

	int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);	// once per second timer
	unsigned long long missed_secs;
	struct itimerspec ts;
	ts.it_interval.tv_sec = 1;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 1;
	ts.it_value.tv_nsec = 0;
	timerfd_settime(timer_fd, 0, &ts, 0);
	
	int beacon_rate = beacon::static_rate;
	float turn_threshold = 0;
	short int hdg_last = 0;
	short int hdg_change = 0;
	int beacon_timer = beacon_rate;									// send startup beacon
	
	while (read(timer_fd, &missed_secs, sizeof(missed_secs))) {		// then send them periodically after that
		// if (debug.verbose && missed_secs > 1) printf("Ticks missed: %lld\n", missed_secs - 1);
		GpsPos gps = gps::getPos();
		
		if ((beacon_timer >= beacon_rate) && gps.valid) {			// if it's time...
			if (debug.sb) printf("SB_DEBUG: Sending beacon.\n");
			beacon::send();
			
			beacon_timer = 0;
			hdg_last = gps.hdg;
		}

		if ((beacon::static_rate == 0) && gps.valid) {		// here we will implement SmartBeaconing(tm) from HamHUD.net
															// see http://www.hamhud.net/hh2/smartbeacon.html for more info
			short int hdg_diff = gps.hdg - hdg_last;
			
			if (gps.speed <= beacon::sb_low_speed) {
				beacon_rate = beacon::sb_low_rate;
			} else if (gps.speed >= beacon::sb_high_speed) {
				beacon_rate = beacon::sb_high_rate;
			} else {
				beacon_rate = beacon::sb_high_rate * beacon::sb_high_speed / gps.speed;
				turn_threshold = beacon::sb_turn_min + beacon::sb_turn_slope / gps.speed;
			
				if (abs(hdg_diff) <= 180) {
					hdg_change = abs(hdg_diff);
				} else if (gps.hdg > hdg_last) {		// thanks to saus on stackoverflow for this nifty solution
					hdg_change = abs(hdg_diff) - 360;
				} else {
					hdg_change = 360 - abs(hdg_diff);
				}
			
				if (abs(hdg_change) > turn_threshold && beacon_timer > beacon::sb_turn_time) beacon_timer = beacon_rate;	// SmartBeaconing spec says CornerPegging is "ALWAYS" enabled, but GPS speed doesn't seem to be accurate enough to keep this from being triggered while stopped.
			}
			
			if (debug.sb) printf("SB_DEBUG: Speed:%.2f Rate:%i Timer:%i LstHdg:%i Hdg:%i HdgChg:%i Thres:%.0f\n", gps.speed, beacon_rate, beacon_timer, hdg_last, gps.hdg, hdg_change, turn_threshold);
			if (console.disp) dprintf(console.iface, "\x1B[5;6H\x1B[KRate:%i Timer:%i LstHdg:%i Hdg:%i HdgChg:%i Thres:%.0f LstHrd:%i", beacon_rate, beacon_timer, hdg_last, gps.hdg, hdg_change, turn_threshold, beacon::last_heard);
		}
		
		if (debug.sb && !gps.valid && gps::enabled) printf("SB_DEBUG: GPS data invalid. Rate:%i Timer:%i\n", beacon_rate, beacon_timer);
		if (console.disp && !gps.valid && gps::enabled) dprintf(console.iface, "\x1B[5;6H\x1B[KGPS data invalid. Rate:%i Timer:%i\n     ", beacon_rate, beacon_timer);
		
		beacon_timer++;
		beacon::last_heard++;		// this will overflow if not reset for 136 years. then again maybe it's not a problem.
	}

	return 0;

}	// END OF 'main'
