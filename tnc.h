#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __TNC_INC
#define __TNC_INC
// INCLUDES
#include <string>
#include <vector>
#include "gpio.h"

// STRUCTS
struct ax25address {				// struct for working with calls
	string callsign;
	unsigned char ssid;
	bool hbit;
	bool last;
	void encode();
	void decode();
};

struct TncStruct
{
	int vhf_iface;					// vhf tnc serial port fd
	unsigned char vhf_kissport;		// vhf tnc kiss port
	int hf_iface;					// hf tnc serial port fd
	unsigned char hf_kissport;		// hf tnc kiss port
	gpio::Led* led;
};

// FUNCTIONS
string encode_ax25_callsign(const char*);
char encode_ax25_ssid(char, bool, bool);
void send_kiss_frame(bool, const char*, unsigned char, const char*, unsigned char, vector<string>, vector<char>, string, vector<bool> = vector<bool>());
void process_ax25_frame(string);
void* tnc_thread(void*);
#endif