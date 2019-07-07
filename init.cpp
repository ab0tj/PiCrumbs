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
#include "INIReader.h"
#include "init.h"
#include "version.h"
#include "hamlib.h"
#include "stringfuncs.h"
#include "beacon.h"
#include "gps.h"
#include "debug.h"
#include "http.h"
#include "gpio.h"
#include "console.h"
#include "tnc.h"
#include "predict.h"
#include "psk.h"

extern PredictStruct predict;
extern HttpStruct http;
extern HamlibStruct hamlib;
extern TncStruct tnc;

DebugStruct debug;

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

int open_port(string name, string port, int baud, bool blocking, bool canon) {			// open a serial port
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

	if (debug.verbose) printf("Successfully opened %s port %s at %i baud\n", name.c_str(), port.c_str(), baud);

	tcgetattr(iface, &options);								// get current control options for the port
	cfsetispeed(&options, baud_code);						// set in baud rate
	cfsetospeed(&options, baud_code);						// set out baud rate
	options.c_cflag &= ~PARENB;								// no parity
	options.c_cflag &= ~CSTOPB;								// 1 stop bit
	options.c_cflag &= ~CSIZE;								// turn off 'csize'
	options.c_cflag |= CS8;									// 8 bit data
	options.c_cflag |= (CLOCAL | CREAD);					// enable the receiver and set local mode
	options.c_oflag &= ~OPOST;								// raw output
	if (canon) {
		options.c_lflag = ECHO | ECHOE | ICANON;			// echo input, set canonical mode
		options.c_iflag |= ICRNL;							// translate CR to NL
	} else {
		options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);	// raw input
	}
	options.c_cc[VERASE] = 8;								// use BS instead of DEL
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
	int tempInt, tempInt1, tempInt2;
	int tempBool;
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
				debug.verbose = true;	// user wants to hear about what's going on
				break;
			case 'z':		// user wants debug info	TODO: should this be documented?
				if (strcmp(optarg, "gps") == 0) debug.gps = true;
				else if (strcmp(optarg, "tnc") == 0) debug.tnc = true;
				else if (strcmp(optarg, "sb") == 0) debug.sb = true;
				else if (strcmp(optarg, "fh") == 0) debug.fh = true;
				else if (strcmp(optarg, "hl") == 0)
				{
					HAMLIB_API::rig_set_debug(RIG_DEBUG_TRACE);
					debug.hl = true;
				}
				else if (strcmp(optarg, "is") == 0) debug.curl = true;
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

	if (debug.verbose) printf("PiCrumbs %s\n\n", VERSION);

// CONFIG FILE PARSING
	INIReader readconfig(configfile);	// read the config ini

	if (readconfig.ParseError() < 0) {	// if we couldn't parse the config
		fprintf(stderr, "Error loading %s\n", configfile.c_str());
		exit (EXIT_FAILURE);
	}

	if (debug.verbose) printf("Using config file %s\n", configfile.c_str());
 // station info
	string call = readconfig.Get("station", "mycall", "N0CALL");	// parse the mycall config paramater
	beacon::mycall = get_call(call);
	beacon::myssid = get_ssid(call);
	if (beacon::mycall.length() > 6) printf("WARNING: Station callsign too long for AX25. It will be truncated to 6 characters if used for packet paths.\n");
	if (beacon::myssid > 15) printf("WARNING: SSID not valid for AX25 (it should be 0-15). Expect weirdness if used on packet paths.\n");
	if (debug.verbose) printf("Operating as %s-%i\n", beacon::mycall.c_str(), beacon::myssid);
// gpio config
	tempInt = readconfig.GetInteger("gpio", "expander", -1);
	tempInt1 = strtol(readconfig.Get("gpio", "expander_addr", "-1").c_str(), NULL, 16);
	tempInt2 = readconfig.GetInteger("gpio", "expander_pinbase", -1);
	if (tempInt != -1) gpio::initExpander((gpio::ExpanderType)tempInt, tempInt1, tempInt2);
	tempInt = readconfig.GetInteger("gpio", "psk_ptt_pin", 65536);
	if (tempInt < 65536) pskPttPin = new gpio::Pin(tempInt, OUTPUT, false);
 // tnc config
	// vhf
	string vhf_tnc_port = readconfig.Get("vhf_tnc", "port", "/dev/ttyS0");
	tnc.vhf_kissport = readconfig.GetInteger("vhf_tnc", "kissport", 0);
	unsigned int vhf_tnc_baud = readconfig.GetInteger("vhf_tnc", "baud", 9600);
	// hf
	bool hf_tnc_enable = readconfig.GetBoolean("hf_tnc", "enable", false);
	string hf_tnc_port = readconfig.Get("hf_tnc", "port", "/dev/ttyS2");
	tnc.hf_kissport = readconfig.GetInteger("hf_tnc", "kissport", 1);
	unsigned int hf_tnc_baud = readconfig.GetInteger("hf_tnc", "baud", 9600);
 // gps config
	gps::enabled = readconfig.GetBoolean("gps", "enable", false);
	tempInt1 = readconfig.GetInteger("gps", "led_pin", 65536);
	tempInt2 = readconfig.GetInteger("gps", "led_pin2", 65536);
	if (tempInt1 < 65536) gps::led = new gpio::Led(tempInt1, tempInt2);
 // console config
	bool console_enable = readconfig.GetBoolean("console", "enable", false);
	string console_port = readconfig.Get("console", "port", "/dev/ttyS4");
	unsigned int console_baud = readconfig.GetInteger("console", "baud", 115200);
 // beacon config
	beacon::comment = readconfig.Get("beacon", "comment", "");
	beacon::compress = readconfig.GetBoolean("beacon", "compressed", false);
	beacon::send_alt = readconfig.GetBoolean("beacon", "send_alt", false);
	beacon::send_course = readconfig.GetBoolean("beacon", "send_course", beacon::compress && !beacon::send_alt);
	beacon::send_speed = readconfig.GetBoolean("beacon", "send_speed", beacon::compress && !beacon::send_alt);
	beacon::symbol_table = readconfig.Get("beacon", "symbol_table", "/")[0];
	beacon::symbol_char = readconfig.Get("beacon", "symbol", "/")[0];
	beacon::temp_file = readconfig.Get("beacon", "temp_file", "");
	beacon::temp_f = readconfig.GetBoolean("beacon", "temp_f", false);
	beacon::adc_file = readconfig.Get("beacon", "adc_file", "");
	beacon::adc_scale = atof(readconfig.Get("beacon", "adc_scale", "1").c_str());
	beacon::static_rate = readconfig.GetInteger("beacon", "static_rate", 900);
	beacon::sb_low_speed = readconfig.GetInteger("beacon", "sb_low_speed", 5);
	beacon::sb_low_rate = readconfig.GetInteger("beacon", "sb_low_rate", 1800);
	beacon::sb_high_speed = readconfig.GetInteger("beacon", "sb_high_speed", 60);
	beacon::sb_high_rate = readconfig.GetInteger("beacon", "sb_high_rate", 180);
	beacon::sb_turn_min = readconfig.GetInteger("beacon", "sb_turn_min", 30);
	beacon::sb_turn_time = readconfig.GetInteger("beacon", "sb_turn_time", 15);
	beacon::sb_turn_slope = readconfig.GetInteger("beacon", "sb_turn_slope", 255);
	tempInt1 = readconfig.GetInteger("beacon", "led_pin", 65536);
	tempInt2 = readconfig.GetInteger("beacon", "led_pin2", 65536);
	if (tempInt1 < 65536) beacon::led = new gpio::Led(tempInt1, tempInt2);
	if (beacon::compress && beacon::send_alt && (beacon::send_course || beacon::send_speed))
	{
		fprintf(stderr, "Configuration error: Compressed beacon supports course and speed or altitude, but not both.\n");
		exit(EXIT_FAILURE);
	}
 // sat tracking config
	predict.path = readconfig.Get("predict", "path", "");
	predict.tlefile = readconfig.Get("predict", "tlefile", "~/.predict/predict.tle");
 // radio control config
	hamlib.enabled = readconfig.GetBoolean("radio", "enable", "false");
	string hamlib_port = readconfig.Get("radio", "port", "/dev/ttyS3");
	string hamlib_baud = readconfig.Get("radio", "baud", "9600");		// hamlib doesn't want an int here
	unsigned short int hamlib_model = readconfig.GetInteger("radio", "model", 1);	// dummy rig as default
	if (hamlib.enabled) beacon::radio_retune = readconfig.GetBoolean("radio", "retune", "false");	// don't try to retune if hamlib is not enabled
 // aprs-is config
	http.enabled = readconfig.GetBoolean("aprsis", "enable", "false");
	http.server = readconfig.Get("aprsis", "server", "rotate.aprs2.net");
	http.port = readconfig.GetInteger("aprsis", "port", 8080);
	http.proxy = readconfig.Get("aprsis", "proxy", "");
	http.proxy_port = readconfig.GetInteger("aprsis", "proxy_port", 1080);
	http.user = readconfig.Get("aprsis", "user", beacon::mycall);
	http.pass = readconfig.Get("aprsis", "pass", "-1");

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
		beacon::aprspath thispath = beacon::aprspath();
		string path_s = pathsect.str();
		thispath.name = readconfig.Get(path_s, "name", path_s);
		thispath.freq = readconfig.GetInteger(path_s, "freq", 144390000);
		thispath.mode = modemap[readconfig.Get(path_s, "mode", "FM")];
		thispath.sat = readconfig.Get(path_s, "sat", "");
		thispath.min_ele = readconfig.GetInteger(path_s, "min_ele", 0);
		thispath.proto = (beacon::PathType)readconfig.GetInteger(path_s, "proto", 0);
		thispath.psk_freq = readconfig.GetInteger(path_s, "psk_freq", 2100);
		thispath.psk_vol = readconfig.GetInteger(path_s, "psk_vol", 100);
		if (thispath.proto == 2) curl_global_init(CURL_GLOBAL_ALL);		// we won't init curl if it's never going to be used
		thispath.retry = readconfig.GetBoolean(path_s, "retry", true);
		thispath.holdoff = readconfig.GetInteger(path_s, "holdoff", 0);
		thispath.comment = readconfig.Get(path_s, "comment", "");
		thispath.usePathComment = readconfig.HasValue(path_s, "comment");
		tempInt = readconfig.GetInteger(path_s, "enablepin", 65536);
		tempBool = readconfig.GetBoolean(path_s, "pullup", false);
		if (tempInt < 65536) thispath.enablePin = new gpio::Pin(tempInt, INPUT, tempBool);
		
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
				if (this_ssid < 0 or this_ssid > 15) {
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

		beacon::aprs_paths.push_back(thispath);		// add path to path vector
		pathidx++;
		pathsect.str(string());	// clear pathsect
		pathsect << "path" << pathidx;
	} while (readconfig.GetInteger(pathsect.str(), "proto", -1) != -1);	// all paths must at least have a proto setting
	if (debug.verbose) printf("Found %i APRS paths.\n", (int)beacon::aprs_paths.size());

// OPEN TNC INTERFACE(s)

	// no 'if' for vhf, since this would be pointless without at least a VHF TNC	TODO: HF-only compatibility
	tnc.vhf_iface = open_port("VHF TNC", vhf_tnc_port, vhf_tnc_baud, true);
	
	if (hf_tnc_enable) {
		if (hf_tnc_port.compare(vhf_tnc_port) == 0) {
			tnc.hf_iface = tnc.vhf_iface;
		} else {
			tnc.hf_iface = open_port("HF TNC", hf_tnc_port, hf_tnc_baud, true);
		}
	}

// OPEN RIG INTERFACE
	if (hamlib.enabled) {
		try {
			hamlib.radio = new Rig(hamlib_model);
			hamlib.radio->setConf("rig_pathname", hamlib_port.c_str());
			hamlib.radio->setConf("serial_speed", hamlib_baud.c_str());
			hamlib.radio->open();
			if (debug.verbose) printf("Successfully opened radio port %s at %s baud\n", hamlib_port.c_str(), hamlib_baud.c_str());
		} catch(RigException& e) {
			fprintf(stderr, "Hamlib exception when opening rig: %i (%s)\n", e.errorno, e.message);
			exit (EXIT_FAILURE);
		}
	}
	
// OPEN CONSOLE INTERFACE
	if (console_enable) console::iface = open_port("console", console_port, console_baud, true, true);

// SET INITAL VALUES
	beacon::last_heard = 999;
	
	if (debug.verbose) printf("Init finished!\n\n");
}	// END OF 'init'
