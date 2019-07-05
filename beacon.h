#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __BEACON_DEF
#define __BEACON_DEF
// INCLUDES
#include <vector>
#include <rigclass.h>
#include "gpio.h"

// ENUMS
enum PathType { VHF_AX25 = 0, HF_AX25 = 1, APRS_IS = 2, PSK63 = 3, PSKAndAX25 = 4 };

// STRUCTS
struct aprspath {
	string name;							// User-defined path name
	freq_t freq;							// frequency of this path
	rmode_t mode;							// mode of this path
	string sat;								// sat name to look up with PREDICT
	unsigned char min_ele;					// minimum elevation of sat before trying to use it
	PathType proto;							// APRS protocol
	bool last_psk;							// did we use psk last time?
	bool retry;								// try again before moving on?
	vector<string> pathcalls;				// path callsigns
	vector<char> pathssids;					// path ssids
	unsigned int holdoff;					// wait at least this many seconds before reusing this path
	time_t lastused;						// last time we sent a packet on this path
	unsigned int attempt;					// number of times we have tried to use this path
	unsigned int success;					// number of times we sucessfully sent a beacon on this path
	unsigned int psk_freq;					// psk63 audio frequency
	unsigned int psk_vol;					// psk63 volume;
	string comment;							// text to send in comment field
	bool usePathComment;					// true = use this comment instead of default
	gpio::Pin* enablePin;					// GPIO pin to check to see if path is enabled
};

namespace beacon
{
	extern string mycall;					// callsign we're operating under, excluding ssid
	extern unsigned char myssid;			// ssid of this station (stored as a number, not ascii)
	extern bool compress_pos;				// should we compress the aprs packet?
	extern char symbol_table;				// which symbol table to use
	extern char symbol_char;				// which symbol to use from the table
	extern string comment;					// comment to send along with aprs packets
	extern vector<aprspath> aprs_paths;		// APRS paths to try, in order of preference
	extern unsigned int last_heard;			// time since we heard our call on vhf
	extern bool radio_retune;				// should we retune the radio after beaconing?
	extern unsigned short int sb_low_speed;	// SmartBeaconing low threshold, in mph
	extern unsigned int sb_low_rate;		// SmartBeaconing low rate
	extern unsigned short int sb_high_speed; // SmartBeaconing high threshold, in mph
	extern unsigned int sb_high_rate;		// SmartBeaconing high rate
	extern unsigned short int sb_turn_min;	// SmartBeaconing turn minimum (deg)
	extern unsigned short int sb_turn_time;	// SmartBeaconing turn time (minimum)
	extern unsigned short int sb_turn_slope; // SmartBeaconing turn slope
	extern unsigned int static_rate;		// how often (in seconds) to send a beacon if not using gps, set to 0 for SmartBeaconing
    extern string temp_file;				// file to get temperature info from, blank to disable
    extern bool temp_f;						// temp units: false for C, true for F
	extern string adc_file;					// file to get ADC value from, blank to disable
	extern float adc_scale;					// ADC scaling value
	extern gpio::Led* led;					// LED to display beacon status

	bool send_pos_report(aprspath&);
	int send();
};

#endif
