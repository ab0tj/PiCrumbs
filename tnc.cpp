#include "tnc.h"
#include <string>
#include <cstring>
#include <bitset>
#include <vector>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include "beacon.h"
#include "stringfuncs.h"
#include "console.h"

// GLOBAL VARS
extern bool tnc_debug;				// rf debugging
extern string mycall;				// callsign we're operating under, excluding ssid
extern unsigned char myssid;		// ssid of this station (stored as a number, not ascii)
extern unsigned int last_heard;		// time since we heard a station on vhf
extern bool console_disp;				// print smartbeaconing params to console
extern int console_iface;				// console serial port fd

// VARS
int vhf_tnc_iface;					// vhf tnc serial port fd
unsigned char vhf_tnc_kissport;			// vhf tnc kiss port
int hf_tnc_iface;					// hf tnc serial port fd
unsigned char hf_tnc_kissport;			// hf tnc kiss port

void ax25address::decode() {					// transform ax25 address into plaintext
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
} // END OF 'ax25address::decode'

void ax25address::encode() {					// transform plaintext data into ax25 address
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
}	// END OF 'ax25address::encode'

string encode_ax25_callsign(const char* callsign) {		// pad a callsign with spaces to 6 chars and shift chars to the left
	char paddedcallsign[] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00};	// start with a string of spaces
	memcpy(paddedcallsign, callsign, strlen(callsign));		// copy the shifted callsign into our padded container
	for (int i=0;i<6;i++) {
			paddedcallsign[i] <<= 1;		// shift all the chars in the input callsign
		}
	return paddedcallsign;
}	// END OF 'encode_ax25_callsign'

char encode_ax25_ssid(char ssid, bool hbit, bool last) {	// format an ax25 ssid byte
	ssid <<= 1;			// shift ssid
	if (hbit) {			// set h and c bits
		ssid |= 0xE0;	// 11100000
	} else {
		ssid |= 0x60;	// 01100000
	}
	ssid |= last;		// set address end bit
	return ssid;
} // END OF 'encode_ax25_ssid'

void send_kiss_frame(bool hf, const char* source, int source_ssid, const char* destination, int destination_ssid, vector<string> via, vector<char>via_ssids, string payload, vector<bool>via_hbits) {		// send a KISS packet to the TNC
	// we'll build the ax25 frame before adding the kiss encapsulation
	string buff = encode_ax25_callsign(destination);					// add destination address
	buff.append(1, encode_ax25_ssid(destination_ssid, false, false));	// add destination ssid
	buff.append(encode_ax25_callsign(source));							// add source address
	if (via.size() == 0) {
		buff.append(1, encode_ax25_ssid(source_ssid, false, true));	// no path, add source ssid and end the address field
	} else {
		if (via_hbits.size() == 0) {							// via_hbits was not specified, fill it with zeros
			for (unsigned int i=0;i<via.size();i++) {
				via_hbits.push_back(false);
			}
		}
		buff.append(1, encode_ax25_ssid(source_ssid, false, false));	// path to follow, don't end the address field just yet
		for (unsigned int i=0;i<(via.size()-1);i++) {					// loop thru all via calls except the last
			buff.append(encode_ax25_callsign(via[i].c_str()));			// add this via callsign
			buff.append(1, encode_ax25_ssid(via_ssids[i], via_hbits[i], false)); // add this via ssid
		}
		buff.append(encode_ax25_callsign(via[via.size()-1].c_str()));	// finally made it to the last via call
		buff.append(1,encode_ax25_ssid(via_ssids[via_ssids.size()-1], via_hbits[via_hbits.size()-1], true));	// end the address field
	}
	
	buff.append("\x03\xF0");									// add control and pid bytes (ui frame)
	buff.append(payload);										// add the actual data
	// now we can escape any FENDs and FESCs that appear in the ax25 frame and add kiss encapsulation
	find_and_replace(buff, "\xDB", "\xDB\xDD");					// replace any FESCs with FESC,TFESC
	find_and_replace(buff, "\xC0", "\xDB\xDC");					// replace any FENDs with FESC,TFEND
	
	buff.insert(0, 1, 0xC0);									// add kiss header
	
	if (hf) {
		buff.insert(1, 1, hf_tnc_kissport << 4);				// add control byte
	} else {
		buff.insert(1, 1, vhf_tnc_kissport << 4);
	}
	
	buff.append(1, 0xC0);										// add kiss footer
	
	if (hf) {
		write(hf_tnc_iface, buff.c_str(), buff.length());		// spit this out the kiss interface
	} else {
		write(vhf_tnc_iface, buff.c_str(), buff.length());
	}
	
	if (tnc_debug) {
		if (hf) {
			printf("TNC_OUT(hf): %s", source);
		} else {
			printf("TNC_OUT(vhf): %s", source);
		}
		if (source_ssid != 0) printf("-%i", source_ssid);
		printf(">%s", destination);
		if (destination_ssid != 0) printf("-%i", destination_ssid);
		for (unsigned int i=0;i<via.size();i++) {
			printf(",%s", via[i].c_str());
			if (via_ssids[i] != 0) printf("-%i", via_ssids[i]);
			if (via_hbits[i]) printf("*");
		}
		printf(":%s\n", payload.c_str());
	}
}	// END OF 'send_kiss_frame'

void process_ax25_frame(string data) {		// listen for our own packets and update last heard var
	ax25address source;

	source.callsign = data.substr(7,6);
	source.ssid = data[13];
	source.decode();

	if (tnc_debug || console_disp) {					// process the rest of the frame and print it out
		ax25address destination;
		vector<ax25address> via;
		stringstream tnc2;

		destination.callsign = data.substr(0,6);
		destination.ssid = data[6];
		destination.decode();
		unsigned int index = 14;
		int viacalls = 0;
		if (!source.last) {		// skip if no digis in address field
			ax25address thisone;
			do {
				if (index > data.length() - 8) break;	// avoid running past the end of the buffer on bad frames
				thisone.callsign = data.substr(index,6);
				thisone.ssid = data[index+6];
				thisone.decode();
				via.push_back(thisone);
				viacalls++;
				index += 7;
			} while (!thisone.last);
		}
		
		tnc2 << StripNonAscii(source.callsign);
		if (source.ssid != 0) tnc2 << '-' << (int)source.ssid;
		tnc2 << '>' << StripNonAscii(destination.callsign);
		if (destination.ssid != 0) tnc2 << '-' << (int)destination.ssid;
		
		for (int i = 0; i < viacalls; i++) {
			tnc2 << ',' << StripNonAscii(via[i].callsign);
			if (via[i].ssid != 0) tnc2 << '-' << (int)via[i].ssid;
			if (via[i].hbit) tnc2 << '*';
		}
		
		if (index < data.length() - 3) tnc2 << ':' << StripNonAscii(data.substr(index+2));
		
		if (tnc_debug) printf("%s\n", tnc2.str().c_str());
		if (console_disp) console_print("\x1B[6;6H\x1B[K" + tnc2.str());
	}

	if ((source.callsign.compare(mycall) == 0) && source.ssid == myssid) {
		if (tnc_debug) printf("TNC_DEBUG: Resetting last_heard. (was %i)\n", last_heard);
		last_heard = 0;	// clear last_heard if we were successfully digi'd.
	}
} // END OF 'process_ax25_frame'

void* tnc_thread(void*) {	// monitor the vhf data stream
	string buff = "";
	unsigned char * data = new unsigned char[1];
	bool escape = false;
	fd_set fds;
	FD_SET(vhf_tnc_iface, &fds);
	for (;;) {
		// select(vhf_tnc_iface + 1, &fds, NULL, NULL, NULL);		// wait for data	TODO: why do nonblocking reads use so much cpu time?
		read(vhf_tnc_iface, data, 1);		// read the data
		if (escape) {
			switch (data[0]) {
				case 0xDC:
					buff.append(1, 0xC0);
					break;
				case 0xDD:
					buff.append(1, 0xDB);
					break;
				default:
					break;
			}
			escape = false;
		} else {
			switch (data[0]) {		// process kiss bytes
				case 0xC0: 				// data is terminated with a FEND.
					if (buff.length() > 20) {		// don't bother if the buffer is smaller than a valid kiss frame
						process_ax25_frame(buff);
					}
					buff = "";
					break;
				case 0xDB:			// FESC
					escape = true;
					break;
				case 0x00:			// control
					break;
				default:			// this wasn't a special char so just add it to the buffer.
					buff.append(1, data[0]);
					break;
			}
		}
	}
	return 0;
} // END OF 'tnc_thread'
