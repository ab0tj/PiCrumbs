#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __BEACON_DEF
#define __BEACON_DEF
// MACROS
#define PACKET_DEST "APRS"				// packet tocall

// INCLUDES
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <unistd.h>
#include "hamlib.h"
#include "pi.h"
#include "http.h"
#include "predict.h"
#include "tnc.h"

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
};

// FUNCTIONS
bool send_pos_report(int);
int beacon();
#endif
