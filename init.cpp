#include "init.h"
#include "INIReader.h"
#include <termios.h>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <vector>
#include <rigclass.h>
#include <curl/curl.h>
#include <wiringPi.h>
#include "version.h"
#include "hamlib.h"
#include "stringfuncs.h"
#include "beacon.h"

// GLOBAL VARS
extern bool compress_pos;						// should we compress the aprs packet?
extern char symbol_table;						// which symbol table to use
extern char symbol_char;						// which symbol to use from the table
extern string beacon_comment;					// comment to send along with aprs packets
extern vector<aprspath> aprs_paths;			// APRS paths to try, in order of preference
extern string mycall;							// callsign we're operating under, excluding ssid
extern unsigned char myssid;					// ssid of this station (stored as a number, not ascii)
extern bool gps_valid;						// should we be sending beacons?
extern unsigned short int sb_low_speed;		// SmartBeaconing low threshold, in mph
extern unsigned int sb_low_rate;				// SmartBeaconing low rate
extern unsigned short int sb_high_speed;		// SmartBeaconing high threshold, in mph
extern unsigned int sb_high_rate;				// SmartBeaconing high rate
extern unsigned short int sb_turn_min;			// SmartBeaconing turn minimum (deg)
extern unsigned short int sb_turn_time;		// SmartBeaconing turn time (minimum)
extern unsigned short int sb_turn_slope;		// SmartBeaconing turn slope
extern unsigned int static_beacon_rate;		// how often (in seconds) to send a beacon if not using gps, set to 0 for SmartBeaconing
extern string temp_file;						// file to get 1-wire temp info from, blank to disable
extern bool temp_f;							// temp units: false for C, true for F
extern unsigned char gpio_hf_en;				// gpio pin for hf enable
extern unsigned char gpio_vhf_en;				// gpio pin for vhf enable
extern string predict_path;					// path to PREDICT program
extern bool gpio_enable;						// can we use gpio pins
extern bool aprsis_enable;					// APRS-IS enable
extern string aprsis_server;					// APRS-IS server name/IP
extern unsigned short int aprsis_port;			// APRS-IS port number
extern string aprsis_proxy;					// HTTP proxy to use for APRS-IS
extern unsigned short int aprsis_proxy_port;	// HTTP proxy port
extern string aprsis_user;						// APRS-IS username/callsign
extern string aprsis_pass;						// APRS-IS password
extern Rig* radio;								// radio control interface reference
extern int vhf_tnc_iface;					// vhf tnc serial port fd
extern unsigned char vhf_tnc_kissport;		// vhf tnc kiss port
extern int hf_tnc_iface;					// hf tnc serial port fd
extern unsigned char hf_tnc_kissport;		// hf tnc kiss port
extern int console_iface;					// console serial port fd
extern unsigned char gpio_psk_ptt;						// gpio pin to use for psk ptt
extern bool radio_retune;					// return to user-set radio frequency/mode after beacon?
extern bool hamlib_enable;					// is radio control enabled?

// VARS
bool verbose;					// did the user ask for verbose mode?
bool sb_debug;					// did the user ask for smartbeaconing info?
bool curl_debug;				// let libcurl show verbose info?
bool fh_debug;					// did the user ask for frequency hopping info?
bool gps_debug;					// did the user ask for gps debug info?
bool hl_debug;					// did the user ask for hamlib debug info?
bool tnc_debug;					// did the user ask for tnc debug info?
bool gps_enable;

int get_baud(int baudint) {		// return a baudrate code from the baudrate int
	switch (baudint) {
		case 0:
			return B0;
		case 50:
			return B50;
		case 75:
			return B75;
		case 110:
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

int open_port(string name, string port, int baud, bool blocking, bool echo) {			// open a serial port
	int baud_code = get_baud(baud);
	if (baud_code == -1) {
		fprintf(stderr, "Invalid %s baud rate %i\n", name.c_str(), baud);
		exit (EXIT_FAILURE);
	}

	struct termios options;
	int iface = open(port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);	// open the port
	
	if (iface == -1) {
		fprintf(stderr, "Could not open %s port %s\n", name.c_str(), port.c_str());
		exit (EXIT_FAILURE);
	} // failed to open port

	if (verbose) printf("Successfully opened %s port %s at %i baud\n", name.c_str(), port.c_str(), baud);

	tcgetattr(iface, &options);								// get current control options for the port
	cfsetispeed(&options, baud_code);						// set in baud rate
	cfsetospeed(&options, baud_code);						// set out baud rate
	options.c_cflag &= ~PARENB;								// no parity
	options.c_cflag &= ~CSTOPB;								// 1 stop bit
	options.c_cflag &= ~CSIZE;								// turn off 'csize'
	options.c_cflag |= CS8;									// 8 bit data
	options.c_cflag |= (CLOCAL | CREAD);					// enable the receiver and set local mode
	options.c_lflag &= ~ICANON;								// raw input
	if (echo) options.c_lflag |= ECHO | ECHONL;				// echo input
	options.c_oflag &= ~OPOST;								// raw output
	tcsetattr(iface, TCSANOW, &options);					// set the new options for the port
	if (blocking) {
		fcntl(iface, F_SETFL, 0);							// blocking reads
	} else {
		fcntl(iface, F_SETFL, FNDELAY);						// nonblocking reads
	}
	tcflush(iface, TCIOFLUSH);								// flush buffer
	return iface;											// return the file number
}	// END OF 'open_port'

void init(int argc, char* argv[]) {		// read config, set up serial ports, etc
	string configfile = "/etc/picrumbs.conf";
	HAMLIB_API::rig_set_debug(RIG_DEBUG_NONE);	// tell hamlib to STFU unless asked

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
				else if (strcmp(optarg, "fh") == 0) fh_debug = true;
				else if (strcmp(optarg, "hl") == 0)
				{
					HAMLIB_API::rig_set_debug(RIG_DEBUG_TRACE);
					hl_debug = true;
				}
				else if (strcmp(optarg, "is") == 0) curl_debug = true;
				break;
			case '?':		// can't understand what the user wants from us, let's set them straight
				fprintf(stderr, "Usage: picrumbs [-v] [-c CONFIGFILE] [-z DEBUGOPT]\n\n");
				fprintf(stderr, "Options:\n -v\tbe verbose\n -c\tspecify config file\n -z\tDebugging messages (see below)\n -?\tshow this help\n");
				fprintf(stderr, "\nDebugging options (-z can be specified multiple times):\n tnc\tShow info about data sent and recieved by the TNC\n");
				fprintf(stderr, " sb\tShow SmartBeaconing information\n fh\tShow path debugging information\n");
				fprintf(stderr, " hl\tShow rig control (hamlib) debugging information\n is\tShow APRS-IS (curl) debugging information\n");
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
 // station info
	string call = readconfig.Get("station", "mycall", "N0CALL");	// parse the mycall config paramater
	mycall = get_call(call);
	myssid = get_ssid(call);
	if (mycall.length() > 6) {
		fprintf(stderr,"MYCALL: Station callsign must be 6 characters or less.\n");
		exit (EXIT_FAILURE);
	}
	if (myssid < 0 or myssid > 15) {
		fprintf(stderr,"MYCALL: Station SSID must be between 0 and 15.\n"); // TODO: We don't care about this if it doesn't need to go over-the-air.
		exit (EXIT_FAILURE);
	}
	if (verbose) printf("Operating as %s-%i\n", mycall.c_str(), myssid);
 // tnc config
	// vhf
	string vhf_tnc_port = readconfig.Get("vhf_tnc", "port", "/dev/ttyS0");
	vhf_tnc_kissport = readconfig.GetInteger("vhf_tnc", "kissport", 0);
	unsigned int vhf_tnc_baud = readconfig.GetInteger("vhf_tnc", "baud", 9600);
	// hf
	bool hf_tnc_enable = readconfig.GetBoolean("hf_tnc", "enable", false);
	string hf_tnc_port = readconfig.Get("hf_tnc", "port", "/dev/ttyS2");
	hf_tnc_kissport = readconfig.GetInteger("hf_tnc", "kissport", 1);
	unsigned int hf_tnc_baud = readconfig.GetInteger("hf_tnc", "baud", 9600);
 // gps config
	gps_enable = readconfig.GetBoolean("gps", "enable", false);
 // console config
	bool console_enable = readconfig.GetBoolean("console", "enable", false);
	string console_port = readconfig.Get("console", "port", "/dev/ttyS4");
	unsigned int console_baud = readconfig.GetInteger("console", "baud", 115200);
 // beacon config
	beacon_comment = readconfig.Get("beacon", "comment", "");
	compress_pos = readconfig.GetBoolean("beacon", "compressed", false);
	symbol_table = readconfig.Get("beacon", "symbol_table", "/")[0];
	symbol_char = readconfig.Get("beacon", "symbol", "/")[0];
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
 // sat tracking config
	predict_path = readconfig.Get("predict", "path", "");
 // gpio config
	gpio_enable = readconfig.GetBoolean("gpio", "enable", false);
	gpio_hf_en = readconfig.GetInteger("gpio", "hf_en_pin", 5);
	gpio_vhf_en = readconfig.GetInteger("gpio", "vhf_en_pin", 6);
	gpio_psk_ptt = readconfig.GetInteger("gpio", "psk_ptt_pin", 7);

	if (gpio_enable) {	// set up GPIO stuff
		wiringPiSetup();
		pinMode(gpio_hf_en, INPUT);				// set pin to input
		pullUpDnControl(gpio_hf_en, PUD_UP);
		pinMode(gpio_vhf_en, INPUT);
		pullUpDnControl(gpio_vhf_en, PUD_UP);
		pinMode(gpio_psk_ptt, OUTPUT);
		digitalWrite(gpio_psk_ptt, 1);	// ptt is active low
	}
 // radio control config
	hamlib_enable = readconfig.GetBoolean("radio", "enable", "false");
	string hamlib_port = readconfig.Get("radio", "port", "/dev/ttyS3");
	string hamlib_baud = readconfig.Get("radio", "baud", "9600");		// hamlib doesn't want an int here
	unsigned short int hamlib_model = readconfig.GetInteger("radio", "model", 1);	// dummy rig as default
	if (hamlib_enable) radio_retune = readconfig.GetBoolean("radio", "retune", "false");	// don't try to retune if hamlib is not enabled
 // aprs-is config
	aprsis_enable = readconfig.GetBoolean("aprsis", "enable", "false");
	aprsis_server = readconfig.Get("aprsis", "server", "rotate.aprs2.net");
	aprsis_port = readconfig.GetInteger("aprsis", "port", 8080);
	aprsis_proxy = readconfig.Get("aprsis", "proxy", "");
	aprsis_proxy_port = readconfig.GetInteger("aprsis", "proxy_port", 1080);
	aprsis_user = readconfig.Get("aprsis", "user", mycall);
	aprsis_pass = readconfig.Get("aprsis", "pass", "-1");
 // path config
	unsigned int pathidx = 1;
	stringstream pathsect;
	pathsect << "path" << "1";
	map<string, rmode_t> modemap;
	modemap["FM"] = RIG_MODE_FM;
	modemap["AM"] = RIG_MODE_AM;
	modemap["USB"] = RIG_MODE_USB;
	modemap["LSB"] = RIG_MODE_LSB;
	modemap["RTTY"] = RIG_MODE_RTTY;
	modemap["RTTYR"] = RIG_MODE_RTTYR;
	modemap["PKTFM"] = RIG_MODE_PKTFM;
	modemap["PKTUSB"] = RIG_MODE_PKTUSB;
	modemap["PKTLSB"] = RIG_MODE_PKTLSB;

	do {		// loop thru all paths in the config file
		aprspath thispath;
		string path_s = pathsect.str();
		thispath.freq = readconfig.GetInteger(path_s, "freq", 144390000);
		thispath.mode = modemap[readconfig.Get(path_s, "mode", "FM")];
		thispath.sat = readconfig.Get(path_s, "sat", "");
		thispath.min_ele = readconfig.GetInteger(path_s, "min_ele", 0);
		thispath.proto = readconfig.GetInteger(path_s, "proto", 0);
		thispath.psk_freq = readconfig.GetInteger(path_s, "psk_freq", 2100);
		thispath.psk_vol = readconfig.GetInteger(path_s, "psk_vol", 100);
		if (thispath.proto == 2) curl_global_init(CURL_GLOBAL_ALL);		// we won't init curl if it's never going to be used
		thispath.retry = readconfig.GetBoolean(path_s, "retry", true);
		thispath.holdoff = readconfig.GetInteger(path_s, "holdoff", 0);
		thispath.comment = readconfig.Get(path_s, "comment", "");
		thispath.usePathComment = readconfig.HasValue(path_s, "comment");
		thispath.attempt = 0;
		thispath.success = 0;
		string beacon_via_str = readconfig.Get(path_s, "via", "");	// now we get to parse the via paramater

		if (beacon_via_str.length() > 0) {	// parse via param, skip if no via was defined
			int current;
			int next = -1;
			string this_call;
			int this_ssid;
			do {
				current = next + 1;
				next = beacon_via_str.find_first_of(",", current);
				string call = beacon_via_str.substr(current, next-current);
				this_call = get_call(call);
				this_ssid = get_ssid(call);
				if (this_call.length() > 6) {
					fprintf(stderr,"VIA: Station callsign must be 6 characters or less.\n");
					exit (EXIT_FAILURE);
				}
				if (this_ssid < 0 or myssid > 15) {
					fprintf(stderr,"VIA: Station SSID must be between 0 and 15.\n");
					exit (EXIT_FAILURE);
				}
				thispath.pathcalls.push_back(this_call);		// input validation ok, add this to the via vectors
				thispath.pathssids.push_back(this_ssid);
			} while (next != -1);
			if (thispath.pathcalls.size() > 8) {
				fprintf(stderr,"VIA: Cannot have more than 8 digis in the path.\n");
				exit (EXIT_FAILURE);
			}
		}

		aprs_paths.push_back(thispath);		// add path to path vector
		pathidx++;
		pathsect.str(string());	// clear pathsect
		pathsect << "path" << pathidx;
	} while (readconfig.GetInteger(pathsect.str(), "proto", -1) != -1);	// all paths must at least have a proto setting
	if (verbose) printf("Found %i APRS paths.\n", (int)aprs_paths.size());

// OPEN TNC INTERFACE(s)

	// no 'if' for vhf, since this would be pointless without at least a VHF TNC	TODO: HF-only compatibility
	vhf_tnc_iface = open_port("VHF TNC", vhf_tnc_port, vhf_tnc_baud, true);
	
	if (hf_tnc_enable) {
		if (hf_tnc_port.compare(vhf_tnc_port) == 0) {
			hf_tnc_iface = vhf_tnc_iface;
		} else {
			hf_tnc_iface = open_port("HF TNC", hf_tnc_port, hf_tnc_baud, true);
		}
	}

// OPEN RIG INTERFACE
	if (hamlib_enable) {
		try {
			radio = new Rig(hamlib_model);
			radio->setConf("rig_pathname", hamlib_port.c_str());
			radio->setConf("serial_speed", hamlib_baud.c_str());
			radio->open();
			if (verbose) printf("Successfully opened radio port %s at %s baud\n", hamlib_port.c_str(), hamlib_baud.c_str());
		} catch(RigException& e) {
			fprintf(stderr, "Hamlib exception when opening rig: %i (%s)\n", e.errorno, e.message);
			exit (EXIT_FAILURE);
		}
	}
	
// OPEN CONSOLE INTERFACE
	if (console_enable) console_iface = open_port("console", console_port, console_baud, true, true);
	
	if (verbose) printf("Init finished!\n\n");
}	// END OF 'init'
