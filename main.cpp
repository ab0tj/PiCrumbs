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

extern TncStruct tnc;

void cleanup(int sign) {	// clean up after catching ctrl-c
	if (debug.verbose) printf("\nCleaning up.\n");
	delete beacon::led;
	delete gps::led;
	delete tnc.led;
	delete pskPttPin;
	close(tnc.vhf_iface);
	close(tnc.hf_iface);
	close(console::iface);
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

	if (console::iface > 0) {	// start the console interface if the port was opened
		pthread_t console_t;
		pthread_create(&console_t, NULL, &console::thread, NULL);
	}

	if (beacon::led != NULL) beacon::led->set(gpio::Red);

	if (gpio::leds.size() > 0)
	{
		pthread_t gpio_t;
		pthread_create(&gpio_t, NULL, &gpio::gpio_thread, NULL);
	}

	int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);	// once per second timer
	unsigned long long missed_secs;
	struct itimerspec ts;
	ts.it_interval.tv_sec = 1;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 1;
	ts.it_value.tv_nsec = 0;
	timerfd_settime(timer_fd, 0, &ts, 0);
	
	uint beacon_rate = beacon::static_rate;
	float turn_threshold = 0;
	short int hdg_last = 0;
	short int hdg_change = 0;
	uint beacon_timer = beacon_rate;					// send startup beacon
	uint status_timer = beacon::status_rate;			// and status report
	
	while (read(timer_fd, &missed_secs, sizeof(missed_secs))) {		// then send them periodically after that
		// if (debug.verbose && missed_secs > 1) printf("Ticks missed: %lld\n", missed_secs - 1);
		beacon_timer += missed_secs;
		beacon::last_heard += missed_secs;

		gps::PosStruct gps = gps::getPos();
		
		if (beacon::status_rate != 0)
		{
			status_timer += missed_secs;
			if (status_timer >= beacon::status_rate)		// time for a status report
			{
				beacon::send(beacon::Status);
				status_timer = 0;
			}
		}

		if ((beacon_timer >= beacon_rate) && gps.valid) {			// time for a position report
			if (debug.sb) printf("SB_DEBUG: Sending beacon.\n");
			beacon::send(beacon::Position);
			beacon_timer = 0;
			hdg_last = gps.hdg;
		}

		if ((beacon::static_rate == 0) && gps.valid) {		// here we will implement SmartBeaconing(tm) from HamHUD.net
															// see http://www.hamhud.net/hh2/smartbeacon.html for more info
			short int hdg_diff = gps.hdg - hdg_last;
			
			if (gps.localSpeed <= beacon::sb_low_speed) {
				beacon_rate = beacon::sb_low_rate;
			} else if (gps.localSpeed >= beacon::sb_high_speed) {
				beacon_rate = beacon::sb_high_rate;
			} else {
				beacon_rate = beacon::sb_high_rate * beacon::sb_high_speed / gps.localSpeed;
				turn_threshold = beacon::sb_turn_min + beacon::sb_turn_slope / gps.localSpeed;
			
				if (abs(hdg_diff) <= 180) {
					hdg_change = abs(hdg_diff);
				} else if (gps.hdg > hdg_last) {		// thanks to saus on stackoverflow for this nifty solution
					hdg_change = abs(hdg_diff) - 360;
				} else {
					hdg_change = 360 - abs(hdg_diff);
				}
			
				if (abs(hdg_change) > turn_threshold && beacon_timer > beacon::sb_turn_time) beacon_timer = beacon_rate;	// SmartBeaconing spec says CornerPegging is "ALWAYS" enabled, but GPS speed doesn't seem to be accurate enough to keep this from being triggered while stopped.
			}
			
			if (debug.sb) printf("SB_DEBUG: Speed:%.2f Rate:%i Timer:%i LstHdg:%i Hdg:%i HdgChg:%i Thres:%.0f StsTmr:%d\n", gps.localSpeed, beacon_rate, beacon_timer, hdg_last, gps.hdg, hdg_change, turn_threshold, status_timer);
			if (console::disp) dprintf(console::iface, "\x1B[5;6H\x1B[KRate:%i Timer:%i LstHdg:%i HdgChg:%i Thres:%.0f LstHrd:%i StsTmr:%d", beacon_rate, beacon_timer, hdg_last, hdg_change, turn_threshold, beacon::last_heard, status_timer);
		}
		
		if (debug.sb && !gps.valid && gps::enabled) printf("SB_DEBUG: GPS data invalid. Rate:%i Timer:%i StsTmr:%d\n", beacon_rate, beacon_timer, status_timer);
		if (console::disp && !gps.valid && gps::enabled) dprintf(console::iface, "\x1B[5;6H\x1B[KGPS data invalid. Rate:%i Timer:%i StsTmr:%d\n     ", beacon_rate, beacon_timer, status_timer);
	}

	return 0;
}	// END OF 'main'
