#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __TNC_INC
#define __TNC_INC
// INCLUDES
#include <string>
#include <cstring>
#include <bitset>
#include <vector>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include "beacon.h"
#include "stringfuncs.h"

// STRUCTS
struct ax25address {				// struct for working with calls
	string callsign;
	unsigned char ssid;
	bool hbit;
	bool last;

	void decode() {					// transform ax25 address into plaintext
		string incall = callsign;
		callsign = "";
		for (int i=0;i<6;i++) {
			incall[i] >>= 1;		// shift this char to the right
			if (incall[i] != 0x20) callsign.append(1,incall[i]);
		}
		bitset<8> ssidbits(ssid);
		last = ssidbits.test(0);
		hbit = ssidbits.test(7);
		ssidbits.reset(7);
		ssidbits.reset(6);
		ssidbits.reset(5);
		ssidbits >>= 1;
		ssid = ssidbits.to_ulong();
	} // END OF 'decode'

	void encode() {					// transform plaintext data into ax25 address
		char paddedcallsign[] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00};	// start with a string of spaces
		memcpy(paddedcallsign, callsign.c_str(), strlen(callsign.c_str()));		// copy the shifted callsign into our padded container
		for (int i=0;i<6;i++) {
			paddedcallsign[i] <<= 1;		// shift all the chars in the input callsign
		}
		callsign = paddedcallsign;
		bitset<8> ssidbits(ssid);
		ssidbits <<= 1;			// shift ssid
		ssidbits.set(0,last);
		ssidbits.set(5,true);
		ssidbits.set(6,true);
		ssidbits.set(7,hbit);
		ssid = ssidbits.to_ulong();
	}	// END OF 'encode'
};

// FUNCTIONS
string encode_ax25_callsign(const char*);
char encode_ax25_ssid(char, bool, bool);
void send_kiss_frame(bool, const char*, int, const char*, int, vector<string>, vector<char>, string, vector<bool> = vector<bool>());
void process_ax25_frame(string);
void* tnc_thread(void*);
#endif