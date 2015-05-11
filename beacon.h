#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __BEACON_DEF
#define __BEACON_DEF
// INCLUDES
#include <rigclass.h>
#include <vector>

// MACROS
#define PACKET_DEST "APRS"				// packet tocall

// STRUCTS
struct aprspath {
	freq_t freq;						// frequency in hz
	rmode_t mode;						// FM, USB, LSB, PKTFM, etc.
	string sat;							// sat name to look up with PREDICT
	unsigned char min_ele;				// minimum elevation of sat before trying to use it
	bool hf;							// true for 300 baud, false for 1200 baud
	bool aprsis;						// APRS-IS (internet) path
	bool retry;							// try again before moving on?
	vector<string> pathcalls;			// path callsigns
	vector<char> pathssids;				// path ssids
	unsigned int holdoff;				// wait at least this many seconds before reusing this path
	time_t lastused;					// last time we sent a packet on this path
	unsigned int attempt;				// number of times we have tried to use this path
	unsigned int success;				// number of times we sucessfully sent a beacon on this path
};

// FUNCTIONS
bool send_pos_report(int);
int beacon();
#endif
