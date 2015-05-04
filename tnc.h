#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __TNC_INC
#define __TNC_INC
// INCLUDES
#include <string>
#include <vector>

// STRUCTS
struct ax25address {				// struct for working with calls
	string callsign;
	unsigned char ssid;
	bool hbit;
	bool last;
	void encode();
	void decode();
};

// FUNCTIONS
string encode_ax25_callsign(const char*);
char encode_ax25_ssid(char, bool, bool);
void send_kiss_frame(bool, const char*, int, const char*, int, vector<string>, vector<char>, string, vector<bool> = vector<bool>());
void process_ax25_frame(string);
void* tnc_thread(void*);
#endif