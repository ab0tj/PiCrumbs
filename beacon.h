#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __BEACON_DEF
#define __BEACON_DEF
// INCLUDES
#include <rigclass.h>
#include <vector>

// STRUCTS
struct aprspath {
	freq_t freq;						// frequency in hz
	rmode_t mode;						// FM, USB, LSB, PKTFM, etc.
	string sat;							// sat name to look up with PREDICT
	unsigned char min_ele;				// minimum elevation of sat before trying to use it
	unsigned char proto;				// packet protocol: 0 = 1200 baud, 1 = 300 baud, 2 = aprs-is, 3 = psk63, 4 = alternate 300bd/psk63
	bool last_psk;						// did we use psk last time?
	bool retry;							// try again before moving on?
	vector<string> pathcalls;			// path callsigns
	vector<char> pathssids;				// path ssids
	unsigned int holdoff;				// wait at least this many seconds before reusing this path
	time_t lastused;					// last time we sent a packet on this path
	unsigned int attempt;				// number of times we have tried to use this path
	unsigned int success;				// number of times we sucessfully sent a beacon on this path
	unsigned int psk_freq;				// psk63 audio frequency
	unsigned int psk_vol;				// psk63 volume;
	string comment;						// text to send in comment field
	bool usePathComment;				// true = use this comment instead of default
};

struct BeaconStruct
{
	string mycall;							// callsign we're operating under, excluding ssid
	unsigned char myssid;					// ssid of this station (stored as a number, not ascii)
	bool compress_pos;						// should we compress the aprs packet?
	char symbol_table;						// which symbol table to use
	char symbol_char;						// which symbol to use from the table
	string comment;							// comment to send along with aprs packets
	vector<aprspath> aprs_paths;			// APRS paths to try, in order of preference
	unsigned int last_heard;				// time since we heard our call on vhf
	bool gpio_enable;						// can we use gpio pins
	bool radio_retune;						// should we retune the radio after beaconing?
	unsigned short int sb_low_speed;		// SmartBeaconing low threshold, in mph
	unsigned int sb_low_rate;				// SmartBeaconing low rate
	unsigned short int sb_high_speed;		// SmartBeaconing high threshold, in mph
	unsigned int sb_high_rate;				// SmartBeaconing high rate
	unsigned short int sb_turn_min;			// SmartBeaconing turn minimum (deg)
	unsigned short int sb_turn_time;		// SmartBeaconing turn time (minimum)
	unsigned short int sb_turn_slope;		// SmartBeaconing turn slope
	unsigned int static_rate;		// how often (in seconds) to send a beacon if not using gps, set to 0 for SmartBeaconing
};

// FUNCTIONS
bool send_pos_report(int);
int sendBeacon();
#endif
