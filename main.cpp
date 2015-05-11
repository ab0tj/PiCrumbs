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

//GLOBAL VARS
extern bool gps_enable;					// gps enabled?
extern bool gps_valid;					// should we be sending beacons?
extern float gps_speed;					// speed from gps, in knots
extern short int gps_hdg;				// heading from gps
extern int vhf_tnc_iface;				// vhf tnc serial port fd
extern int hf_tnc_iface;				// hf tnc serial port fd
extern int console_iface;				// console serial port fd
extern Rig* radio;						// radio control interface reference
extern bool verbose;					// verbose interface?
extern bool sb_debug;					// smartbeaconing debug
extern unsigned int last_heard;			// time since we heard our call on vhf
extern bool console_sb;					// print smartbeaconing params to console

// VARS
unsigned short int sb_low_speed;		// SmartBeaconing low threshold, in mph
unsigned int sb_low_rate;				// SmartBeaconing low rate
unsigned short int sb_high_speed;		// SmartBeaconing high threshold, in mph
unsigned int sb_high_rate;				// SmartBeaconing high rate
unsigned short int sb_turn_min;			// SmartBeaconing turn minimum (deg)
unsigned short int sb_turn_time;		// SmartBeaconing turn time (minimum)
unsigned short int sb_turn_slope;		// SmartBeaconing turn slope
unsigned int static_beacon_rate;		// how often (in seconds) to send a beacon if not using gps, set to 0 for SmartBeaconing

void cleanup(int sign) {	// clean up after catching ctrl-c
	if (verbose) printf("\nCleaning up.\n");
	close(vhf_tnc_iface);
	close(hf_tnc_iface);
	close(console_iface);
	radio->close();
	curl_global_cleanup();
	exit (EXIT_SUCCESS);
} // END OF 'cleanup'

int main(int argc, char* argv[]) {
	signal(SIGINT,&cleanup);	// catch ctrl-c

	init(argc, argv);	// get everything ready to go

	pthread_t tnc_t;
	pthread_create(&tnc_t, NULL, &tnc_thread, NULL);

	if (gps_enable) {
		pthread_t gps_t;
		pthread_create(&gps_t, NULL, &gps_thread, NULL);	// start the gps interface thread if the gps interface was opened
	}

	if (console_iface > 0) {	// start the console interface if the port was opened
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
	
	int beacon_rate = static_beacon_rate;
	float turn_threshold = 0;
	short int hdg_last = 0;
	short int hdg_change = 0;
	int beacon_timer = beacon_rate;						// send startup beacon
	while (read(timer_fd, &missed_secs, sizeof(missed_secs))) {								// then send them periodically after that
		// if (verbose && missed_secs > 1) printf("Ticks missed: %lld\n", missed_secs - 1);
		
		if ((beacon_timer >= beacon_rate) && gps_valid) {		// if it's time...
			if (sb_debug) printf("SB_DEBUG: Sending beacon.\n");
			beacon();
			
			beacon_timer = 0;
			hdg_last = gps_hdg;
		}

		if ((static_beacon_rate == 0) && gps_valid) {		// here we will implement SmartBeaconing(tm) from HamHUD.net
															// see http://www.hamhud.net/hh2/smartbeacon.html for more info
			short int hdg_curr = gps_hdg;	// capture heading in case it happens to change during this calculation
			short int hdg_diff = hdg_curr - hdg_last;
			
			if (gps_speed <= sb_low_speed) {
				beacon_rate = sb_low_rate;
			} else if (gps_speed >= sb_high_speed) {
				beacon_rate = sb_high_rate;
			} else {
				beacon_rate = sb_high_rate * sb_high_speed / gps_speed;
				turn_threshold = sb_turn_min + sb_turn_slope / gps_speed;
			
				if (abs(hdg_diff) <= 180) {
					hdg_change = abs(hdg_diff);
				} else if (hdg_curr > hdg_last) {		// thanks to saus on stackoverflow for this nifty solution
					hdg_change = abs(hdg_diff) - 360;
				} else {
					hdg_change = 360 - abs(hdg_diff);
				}
			
				if (abs(hdg_change) > turn_threshold && beacon_timer > sb_turn_time) beacon_timer = beacon_rate;
			}
			
			if (sb_debug) printf("SB_DEBUG: Speed:%.2f Rate:%i Timer:%i LstHdg:%i Hdg:%i HdgChg:%i Thres:%.0f\n", gps_speed, beacon_rate, beacon_timer, hdg_last, hdg_curr, hdg_change, turn_threshold);
			if (console_sb) dprintf(console_iface, "\rSpeed:%.2f Rate:%i Timer:%i LstHdg:%i Hdg:%i HdgChg:%i Thres:%.0f     ", gps_speed, beacon_rate, beacon_timer, hdg_last, hdg_curr, hdg_change, turn_threshold);
		}
		
		if (sb_debug && !gps_valid && gps_enable) printf("SB_DEBUG: GPS data invalid. Rate:%i Timer:%i\n", beacon_rate, beacon_timer);
		
		beacon_timer++;
		last_heard++;		// this will overflow if not reset for 136 years. then again maybe it's not a problem.
	}

	return 0;

}	// END OF 'main'
