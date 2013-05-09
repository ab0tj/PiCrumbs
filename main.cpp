#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <cmath>
#include <bitset>
//#include <hamlib/rig.h>	TODO: rig control
#include "INIReader.cpp"

// DEFINES GO HERE
#define VERSION "0.1"				// program version for messages, etc
#define PACKET_DEST "APMGP1"		// packet tocall

using namespace std;

// GLOBAL VARS GO HERE
string mycall;						// callsign we're operating under, excluding ssid
unsigned char myssid;				// ssid of this station (stored as a number, not ascii)
bool compress_pos;					// should we compress the aprs packet?
string symbol_table;				// which symbol table to use
string symbol_char;					// which symbol to use from the table
unsigned short int sb_low_speed;	// SmartBeaconing low threshold, in mph
unsigned int sb_low_rate;			// SmartBeaconing low rate
unsigned short int sb_high_speed;	// SmartBeaconing high threshold, in mph
unsigned int sb_high_rate;			// SmartBeaconing high rate
unsigned short int sb_turn_min;		// SmartBeaconing turn minimum (deg)
unsigned short int sb_turn_time;	// SmartBeaconing turn time (minimum)
short int sb_turn_slope;			// SmartBeaconing turn slope
bool verbose = false;				// did the user ask for verbose mode?
bool gps_debug = false;				// did the user ask for gps debug info?
bool tnc_debug = false;				// did the user ask for tnc debug info?
bool sb_debug = false;				// did the user ask for smartbeaconing info?
int kiss_vhf_iface = -1;			// vhf tnc serial port fd
int kiss_hf_iface = -1;				// hf tnc serial port fd
int gps_iface = -1;					// gps serial port fd
bool beacon_ok = false;				// should we be sending beacons?
float pos_lat;						// current latitude
string pos_lat_dir;					// latitude direction
float pos_long;						// current longitude
string pos_long_dir;				// current longitude direction
struct tm * gps_time = new tm;		// last time received from the gps (if enabled)
float gps_speed;					// speed from gps, in knots
short int gps_hdg;					// heading from gps
int static_beacon_rate;				// how often (in seconds) to send a beacon if not using gps, set to 0 for SmartBeaconing
string beacon_comment;				// comment to send along with aprs packets
vector<string> path_calls;			// path callsigns
vector<char> path_ssids;			// path ssids
unsigned int last_heard;			// time since we heard a station on vhf
string temp_file;					// file to get 1-wire temp info from, blank to disable
bool temp_f;						// temp units: false for C, true for F

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

// BEGIN FUNCTIONS
void find_and_replace(string& subject, const string& search, const string& replace) {	// find and replace in a string, thanks Czarek Tomczak
	size_t pos = 0;
	while((pos = subject.find(search, pos)) != string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
}

int get_baud(int baudint) {		// return a baudrate code from the baudrate int
	switch (baudint) {
	case 0:
		return B0;
	case 50:
		return B50;
	case 75:
		return B75;
	case 100:
		return B110;
	case 134:
		return B134;
	case 150:
		return B150;
	case 200:
		return B200;
	case 300:
		return B300;
	case 600:
		return B600;
	case 1200:
		return B1200;
	case 1800:
		return B1800;
	case 2400:
		return B2400;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	default:
		return -1;
	}
}	// END OF 'get_baud'

int get_temp() {
	ifstream tempfile;
	string line;
	int temp;
	tempfile.open(temp_file.c_str());
	getline(tempfile, line);
	if (line.substr(36,3).compare("YES") == 0) {
		getline(tempfile, line, '=');
		getline(tempfile, line);
		temp = atoi(line.c_str());
		if (temp_f) temp = 1.8*temp+32000;
		temp /= 1000;
	}
	tempfile.close();
	return temp;
}

int open_port(string port, int baud_code, bool blocking) {			// open a serial port
	struct termios options;
	int iface = open(port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);	// open the port
	if (iface == -1) return -1;								// failed to open port
	tcgetattr(iface, &options);								// get current control options for the port
	cfsetispeed(&options, baud_code);						// set in baud rate
	cfsetospeed(&options, baud_code);						// set out baud rate
	options.c_cflag &= ~PARENB;								// no parity
	options.c_cflag &= ~CSTOPB;								// 1 stop bit
	options.c_cflag &= ~CSIZE;								// turn off 'csize'
	options.c_cflag |= CS8;									// 8 bit data
	options.c_cflag |= (CLOCAL | CREAD);					// enable the receiver and set local mode
	options.c_lflag &= ~ICANON;								// raw input
	options.c_oflag &= ~OPOST;								// raw output
	tcsetattr(iface, TCSANOW, &options);					// set the new options for the port
	if (blocking) {
		fcntl(iface, F_SETFL, 0);							// blocking reads
	} else {
		fcntl(iface, F_SETFL, FNDELAY);						// set port to nonblocking reads
	}
	tcflush(iface, TCIOFLUSH);								// flush buffer
	return iface;											// return the file number
}	// END OF 'open_port'

void init(int argc, char* argv[]) {		// read config, set up serial ports, etc
	string configfile = "/etc/picrumbs.conf";

// COMMAND LINE ARGUMENT PARSING
	
	if (argc > 1) {		// user entered command line arguments
		int c;
		opterr = 0;
		while ((c = getopt (argc, argv, "c:vz:")) != -1)		// loop through all command line args
			switch (c) {
			case 'c':		// user specified a config file
				configfile = optarg;
				break;
			case 'v':
				verbose = true;	// user wants to hear about what's going on
				break;
			case 'z':		// user wants debug info	TODO: should this be documented?
				if (strcmp(optarg, "gps") == 0) gps_debug = true;
				else if (strcmp(optarg, "tnc") == 0) tnc_debug = true;
				else if (strcmp(optarg, "sb") == 0) sb_debug = true;
				break;
			case '?':		// can't understand what the user wants from us, let's set them straight
				fprintf(stderr, "Usage: picrumbs [-v] [-c CONFIGFILE]\n\n");
				fprintf(stderr, "Options:\n -v\tbe verbose\n -c\tspecify config file\n -?\tshow this help\n");
				exit (EXIT_FAILURE);
				break;
			}
	}

	if (verbose) printf("PiCrumbs %s\n\n", VERSION);

// CONFIG FILE PARSING

	INIReader readconfig(configfile);	// read the config ini

	if (readconfig.ParseError() < 0) {	// if we couldn't parse the config
		fprintf(stderr, "Error loading %s\n", configfile.c_str());
		exit (EXIT_FAILURE);
	}

	if (verbose) printf("Using config file %s\n", configfile.c_str());

	string call = readconfig.Get("station", "mycall", "N0CALL");	// parse the mycall config paramater
	int index = call.find_first_of("-");
	if (index == -1) {	// no ssid specified
		mycall = call;
		myssid = 0;
	} else {			// ssid was specified
		mycall = call.substr(0,index);	// left of the dash
		myssid = atoi(call.substr(index+1,2).c_str());	// right of the dash
	}
	if (mycall.length() > 6) {
		fprintf(stderr,"MYCALL: Station callsign must be 6 characters or less.\n");
		exit (EXIT_FAILURE);
	}
	if (myssid < 0 or myssid > 15) {
		fprintf(stderr,"MYCALL: Station SSID must be between 0 and 15.\n");
		exit (EXIT_FAILURE);
	}
	if (verbose) printf("Operating as %s-%i\n", mycall.c_str(), myssid);		// whew, we made it through all the tests

	bool gps_enable = readconfig.GetBoolean("gps", "enable", false);
	string kiss_port = readconfig.Get("tnc", "port", "/dev/ttyS0");
	int kiss_baud = readconfig.GetInteger("tnc", "baud", 9600);
	string gps_port  = readconfig.Get("gps", "port", "/dev/ttyS1");
	int gps_baud = readconfig.GetInteger("gps", "baud", 4800);

	string beacon_via_str = readconfig.Get("beacon", "via", "");		// now we get to parse the via paramater
	if (beacon_via_str.length() > 0) {
		int current;
		int next = -1;
		string this_call;
		int this_ssid;
		do {
			current = next + 1;
			next = beacon_via_str.find_first_of(",", current);
			call = beacon_via_str.substr(current, next-current);
			index = call.find_first_of("-");
			if (index == -1) {	// no ssid specified
				this_call = call;
				this_ssid = 0;
			} else {			// ssid was specified
				this_call = call.substr(0,index);	// left of the dash
				this_ssid = atoi(call.substr(index+1,2).c_str());	// right of the dash
			}
			if (this_call.length() > 6) {
				fprintf(stderr,"VIA: Station callsign must be 6 characters or less.\n");
				exit (EXIT_FAILURE);
			}
			if (this_ssid < 0 or myssid > 15) {
				fprintf(stderr,"VIA: Station SSID must be between 0 and 15.\n");
				exit (EXIT_FAILURE);
			}
			path_calls.push_back(this_call);		// input validation ok, add this to the via vectors
			path_ssids.push_back(this_ssid);
		} while (next != -1);
		if (path_calls.size() > 8) {
			fprintf(stderr,"VIA: Cannot have more than 8 digis in the path.\n");
			exit (EXIT_FAILURE);
		}
	}
	beacon_comment = readconfig.Get("beacon", "comment", "");
	compress_pos = readconfig.GetBoolean("beacon", "compressed", false);
	symbol_table = readconfig.Get("beacon", "symbol_table", "/");
	symbol_char = readconfig.Get("beacon", "symbol", "/");
	temp_file = readconfig.Get("beacon", "temp_file", "");
	temp_f = readconfig.GetBoolean("beacon", "temp_f", false);
	static_beacon_rate = readconfig.GetInteger("beacon", "static_rate", 900);
	sb_low_speed = readconfig.GetInteger("beacon", "sb_low_speed", 5);
	sb_low_rate = readconfig.GetInteger("beacon", "sb_low_rate", 1800);
	sb_high_speed = readconfig.GetInteger("beacon", "sb_high_speed", 60);
	sb_high_rate = readconfig.GetInteger("beacon", "sb_high_rate", 180);
	sb_turn_min = readconfig.GetInteger("beacon", "sb_turn_min", 30);
	sb_turn_time = readconfig.GetInteger("beacon", "sb_turn_time", 15);
	sb_turn_slope = readconfig.GetInteger("beacon", "sb_turn_slope", 255);

// OPEN KISS INTERFACE

	// no 'if' here, since this would be pointless without a TNC

	int baud_code = get_baud(kiss_baud);

	if (baud_code == -1) {		// get_baud says that's an invalid baud rate
		fprintf(stderr, "Invalid KISS baud rate %i\n", kiss_baud);
		exit (EXIT_FAILURE);
	}

	kiss_vhf_iface = open_port(kiss_port, baud_code, false);	// use nonblocking reads for soundmodem compatibility

	if (kiss_vhf_iface == -1) {		// couldn't open the serial port...
		fprintf(stderr, "Could not open KISS port %s\n", kiss_port.c_str());
		exit (EXIT_FAILURE);
	}

	if (verbose) printf("Successfully opened KISS port %s at %i baud\n", kiss_port.c_str(), kiss_baud);

// OPEN GPS INTERFACE

	if (gps_enable) {
		baud_code = get_baud(gps_baud);

		if (baud_code == -1) {
			fprintf(stderr, "Invalid GPS baud rate %i\n", gps_baud);
			exit (EXIT_FAILURE);
		}

		gps_iface = open_port(gps_port, baud_code, true);	// blocking reads work here

		if (gps_iface == -1) {
			fprintf(stderr, "Could not open GPS port %s\n", gps_port.c_str());
			exit (EXIT_FAILURE);
		}

		if (verbose) printf("Successfully opened GPS port %s at %i baud\n", gps_port.c_str(), gps_baud);
	} else beacon_ok = true;	// gps not enabled, use static beacons
	if (verbose) printf("Init finished!\n\n");
}	// END OF 'init'

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

void process_ax25_frame(string data) {		// update last heard var
	ax25address destination;
	ax25address source;
	vector<ax25address> via;
	string payload;
	destination.callsign = data.substr(0,6);
	destination.ssid = data[6];
	destination.decode();
	source.callsign = data.substr(7,6);
	source.ssid = data[13];
	source.decode();
	int index = 14;
	int viacalls = 0;
	if (!source.last) {
		ax25address thisone;
		while (true) {
			thisone.callsign = data.substr(index,6);
			thisone.ssid = data[index+6];
			thisone.decode();
			via.push_back(thisone);
			viacalls++;
			index += 7;
			if (thisone.last) break;
		}
	}
	payload = data.substr(index+2, string::npos);		// all the way to the end of the frame, but skip control bytes
	if (tnc_debug) {									// spit out this packet to the user in tnc2 format
		string viastr;
		char * viatemp = new char[8];
		for (int i=0;i<viacalls;i++) {
			sprintf(viatemp, ",%s", via[i].callsign.c_str());
			viastr.append(viatemp);
			if (via[i].ssid != 0) {						// add the ssid only if it's not 0
				sprintf(viatemp, "-%i", via[i].ssid);
				viastr.append(viatemp);
			}
			if (via[i].hbit) viastr.append("*");
		}
		delete viatemp;
		printf("TNC_IN: %s", source.callsign.c_str());
		if (source.ssid != 0) printf("-%i", source.ssid);
		printf(">%s", destination.callsign.c_str());
		if (destination.ssid != 0) printf("-%i", destination.ssid);
		printf("%s:%s\n", viastr.c_str(), payload.c_str());
	}
	if ((source.callsign.compare(mycall) == 0) && source.ssid == myssid) {
		if (tnc_debug) printf("TNC_DEBUG: Resetting last_heard. (was %i)\n", last_heard);
		last_heard = 0;	// clear last_heard if we were successfully digi'd.
	}
} // END OF 'process_ax25_frame'

void send_kiss_frame(const char* source, int source_ssid, const char* destination, int destination_ssid, vector<string> via, vector<char>via_ssids, string payload, vector<bool>via_hbits = vector<bool>()) {		// send a KISS packet to the TNC
	// we'll build the ax25 frame before adding the kiss encapsulation
	string buff = encode_ax25_callsign(destination);					// add destination address
	buff.append(1, encode_ax25_ssid(destination_ssid, false, false));	// add destination ssid
	buff.append(encode_ax25_callsign(source));							// add source address
	if (via.size() == 0) {
		buff.append(1, encode_ax25_ssid(source_ssid, false, true));	// no path, add source ssid and end the address field
	} else {
		if (via_hbits.size() == 0) {							// via_hbits was not specified, fill it with zeros
			for (int i=0;i<via.size();i++) {
				via_hbits.push_back(false);
			}
		}
		buff.append(1, encode_ax25_ssid(source_ssid, false, false));	// path to follow, don't end the address field just yet
		for (int i=0;i<(via.size()-1);i++) {					// loop thru all via calls except the last
			buff.append(encode_ax25_callsign(via[i].c_str()));			// add this via callsign
			buff.append(1, encode_ax25_ssid(via_ssids[i], via_hbits[i], false)); // add this via ssid
		}
		buff.append(encode_ax25_callsign(via[via.size()-1].c_str()));	// finally made it to the last via call
		buff.append(1,encode_ax25_ssid(via_ssids[via_ssids.size()-1], via_hbits[via_hbits.size()-1], true));	// end the address field
	}
	buff.append("\x03\xF0");									// add control and pid bytes (ui frame)
	buff.append(payload);										// add the actual data
	// now we can escape any FENDs and FESCs that appear in the ax25 frame and add kiss encapsulation
	find_and_replace(buff,"\xDB","\xDB\xDD");					// replace any FESCs with FESC,TFESC
	find_and_replace(buff,"\xC0","\xDB\xDC");					// replace any FENDs with FESC,TFEND
	buff.insert(0,"\xC0\x00",2);								// add kiss header
	buff.append(1,0xC0);										// add kiss footer
	write(kiss_vhf_iface,buff.c_str(),buff.length());				// spit this out the kiss interface
	if (tnc_debug) {
		printf("TNC_OUT: %s", source);
		if (source_ssid != 0) printf("-%i", source_ssid);
		printf(">%s", destination);
		if (destination_ssid != 0) printf("-%i", destination_ssid);
		for (int i=0;i<via.size();i++) {
			printf(",%s", via[i].c_str());
			if (via_ssids[i] != 0) printf("-%i", via_ssids[i]);
			if (via_hbits[i]) printf("*");
		}
		printf(":%s\n", payload.c_str());
	}
}	// END OF 'send_kiss_frame'

void send_pos_report() {		// exactly what it sounds like
	char* pos = new char[21];
	if (compress_pos) {		// build compressed position report, yes, byte by byte.
		pos[0] = 0x21;		// '!'
		pos[1] = symbol_table[0];
		float lat;
		float lon;
		float lat_min = modf(pos_lat/100, &lat);	// separate deg and min
		float lon_min = modf(pos_long/100, &lon);
		lat += (lat_min/.6);	// convert min to deg and re-add it
		lon += (lon_min/.6);
		if (strcmp(pos_lat_dir.c_str(), "S") == 0) lat = -lat;	// assign direction sign
		if (strcmp(pos_long_dir.c_str(), "W") == 0) lon = -lon;
		lat = 380926 * (90 - lat);		// formula from aprs spec
		lon = 190463 * (180 + lon);
		pos[2] = (int)lat / 753571 + 33;	// lat/91^3+33
		lat = (int)lat % 753571;			// remainder
		pos[3] = (int)lat / 8281 + 33;		// remainder/91^2+33
		lat = (int)lat % 8281;				// remainder
		pos[4] = (int)lat / 91 + 33;		// remainder/91^1+33
		pos[5] = (int)lat % 91 + 33;		// remainder + 33
		pos[6] = (int)lon / 753571 + 33;
		lon = (int)lon % 753571;
		pos[7] = (int)lon / 8281 + 33;
		lon = (int)lon % 8281;
		pos[8] = (int)lon / 91 + 33;
		pos[9] = (int)lon % 91 + 33;
		pos[10] = symbol_char[0];
		pos[11] = gps_hdg / 4 + 33;
		pos[12] = (int)pow(gps_speed,(float)(1.08 - 1)) + 33;
		pos[13] = 0x5F;			// set "T" byte
		pos[14] = 0x00;			// (null terminated string)
	} else {	// uncompressed packet
		sprintf(pos, "!%.2f%s%s%.2f%s%s", pos_lat, pos_lat_dir.c_str(), symbol_table.c_str(), pos_long, pos_long_dir.c_str(), symbol_char.c_str());
	}
	stringstream buff;
	buff << pos;
	if (temp_file.compare("") != 0) {
		buff << get_temp();
		if (temp_f) {
			buff << "F ";
		} else {
			buff << "C ";
		}
	}
	buff << beacon_comment;
	delete pos;		// memory leak fixed
	send_kiss_frame(mycall.c_str(), myssid, PACKET_DEST, 0, path_calls, path_ssids, buff.str());
}	// END OF 'send_pos_report'

void* gps_thread(void*) {		// thread to listen to the incoming NMEA stream and update our position and time
	string buff = "";
	char * data = new char[1];

	while (true) {
		read(gps_iface, data, 1);
		if (data[0] == '\n') {			// NMEA data is terminated with a newline.
			if (buff.length() > 6) {	// but let's not bother if this was an incomplete line
				//if (gps_debug) printf("GPS_IN: %s\n", buff.c_str());
				if (buff.compare(0, 6, "$GPRMC") == 0) {	// GPRMC has most of the info we care about
					string params[12];
					int current = 7;
					int next = 0;
					for (int a=0; a<12; a++) {		// here we'll split the fields of the GPRMC sentence
						next = buff.find_first_of(',', current);
						params[a] = buff.substr(current, next - current);
						current = next + 1;
					}
					if (params[1].compare("A") == 0) {
						beacon_ok = true;
						gps_time->tm_hour = atoi(params[0].substr(0,2).c_str());
						gps_time->tm_min = atoi(params[0].substr(2,2).c_str());
						gps_time->tm_sec = atoi(params[0].substr(4,2).c_str());
						gps_time->tm_mday = atoi(params[8].substr(0,2).c_str());
						gps_time->tm_mon = atoi(params[8].substr(2,2).c_str()) - 1;		// tm_mon is 0-11
						gps_time->tm_year = atoi(params[8].substr(4,2).c_str()) + 100; 	// tm_year is "years since 1900"
						pos_lat = atof(params[2].c_str());
						pos_lat_dir = params[3];
						pos_long = atof(params[4].c_str());
						pos_long_dir = params[5];
						gps_speed = atof(params[6].c_str());
						gps_hdg = atoi(params[7].c_str());
						if (gps_debug) printf("GPS_DEBUG: Lat:%f%s Long:%f%s MPH:%.2f Hdg:%i Time:%s", pos_lat, pos_lat_dir.c_str(), pos_long, pos_long_dir.c_str(), gps_speed, gps_hdg, asctime(gps_time));
					} else {
						beacon_ok = false;
						if (gps_debug) printf("GPS_DEBUG: data invalid.\n");
					}
				}
			}
			buff = "";		// clear the buffer
		} else {
			buff.append(1, data[0]);	// this wasn't a newline so just add it to the buffer.
		}
	}
	return 0;
} // END OF 'gps_thread'

void* tnc_thread(void*) {	// monitor the vhf data stream
	string buff = "";
	unsigned char * data = new unsigned char[1];
	bool escape = false;
	fd_set fds;
	FD_SET(kiss_vhf_iface, &fds);

	while (true) {
		select(kiss_vhf_iface + 1, &fds, NULL, NULL, NULL);		// wait for data	TODO: why do nonblocking reads use so much cpu time?
		read(kiss_vhf_iface, data, 1);		// read the data
		switch (data[0]) {		// process kiss bytes
		case 0xC0: 				// data is terminated with a FEND.
			if (buff.length() > 20) {		// don't bother if the buffer is smaller than a valid kiss frame
				process_ax25_frame(buff);
			}
			buff = "";
			break;
		case 0xDB:			// FESC
			escape = !escape;
			break;
		case 0xDC:			// TFEND
			if (escape) {
				buff.append(1, 0xC0);
			} else {
				buff.append(1, 0xDC);
			}
			break;
		case 0xDD:			// TFESC
			if (escape) {
				buff.append(1, 0xDB);
			} else {
				buff.append(1, 0xDD);
			}
			break;
		case 0x00:			// control
			break;
		default:			// this wasn't a special char so just add it to the buffer.
			buff.append(1, data[0]);
			break;
		}
	}
	return 0;
} // END OF 'tnc_thread'

void cleanup(int sign) {	// clean up after catching ctrl-c
	if (verbose) printf("\nClosing TNC interface\n");
	close(kiss_vhf_iface);
	if (verbose) printf("Closing GPS interface\n");
	close(gps_iface);
	exit (EXIT_SUCCESS);
} // END OF 'cleanup'

int main(int argc, char* argv[]) {

	signal(SIGINT,&cleanup);	// catch ctrl-c

	init(argc, argv);	// get everything ready to go

	pthread_t tnc_t;
	pthread_create(&tnc_t, NULL, &tnc_thread, NULL);

	if (gps_iface > 0) {
		pthread_t gps_t;
		pthread_create(&gps_t, NULL, &gps_thread, NULL);	// start the gps interface thread if the gps interface was opened
	}

	sleep (1);	// let everything 'settle'

	int beacon_rate = static_beacon_rate;
	float turn_threshold = 0;
	short int last_hdg = gps_hdg;
	short int hdg_change = 0;
	float speed;
	int beacon_timer = beacon_rate;			// send startup beacon
	while (true) {							// then send them periodically after that
		if (beacon_timer >= beacon_rate) {	// if it's time...
			while (!beacon_ok) sleep (1);	// wait if gps data not valid
			send_pos_report();				// send a beacon
			beacon_timer = 0;
			hdg_change = 0;
		}

		if (static_beacon_rate == 0) {		// here we will implement SmartBeaconing(tm) from HamHUD.net
			speed = gps_speed  * 1.15078;	// convert knots to mph
			if (speed < sb_low_speed) {		// see http://www.hamhud.net/hh2/smartbeacon.html for more info
				beacon_rate = sb_low_rate;
			} else if (speed > sb_high_speed) {
				beacon_rate = sb_high_rate;
			} else {
				beacon_rate = sb_high_rate * sb_high_speed / speed;
			}
			turn_threshold = sb_turn_min + sb_turn_slope / speed;
			hdg_change += gps_hdg - last_hdg;
			last_hdg = gps_hdg;
			if (abs(hdg_change) > turn_threshold && beacon_timer > sb_turn_time && speed > 3) beacon_timer = beacon_rate;	// SmartBeaconing spec says corner-pegging is always enabled regardless of speed, but testing shows GPS "wandering" can set this off needlessly while parked
		}
		if (sb_debug) printf("SB_DEBUG: Rate:%i Timer:%i HdgChg:%i Thres:%f\n", beacon_rate, beacon_timer, hdg_change, turn_threshold);

		sleep(1);
		beacon_timer++;
		last_heard++;		// this will overflow if not reset for 136 years. then again maybe it's not a problem.
	}

	return 0;

}	// END OF 'main'
