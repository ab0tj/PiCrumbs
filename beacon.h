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
};

// FUNCTIONS
bool send_pos_report(int);
int beacon();
#endif
