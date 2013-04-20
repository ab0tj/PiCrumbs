#include <iostream>
#include <string>
#include <cstring>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <time.h>
#include <hamlib/rig.h>
#include "INIReader.cpp"

#define VERSION "0.1"

using namespace std;

// GLOBAL VARS GO HERE
string mycall;					// callsign we're operating under, including ssid
bool verbose = false;			// did the user ask for verbose mode?
bool gps_debug = false;			// did the user ask for gps debug info?
bool tnc_debug = false;			// did the user ask for tnc debug info?
int kiss_iface = -1;			// tnc serial port
int gps_iface = -1;				// gps serial port
float pos_lat;					// current latitude - positive N, negative S
float pos_long;					// current longitude - positive W, negative E
struct tm * gps_time = new tm;			// last time received from the gps (if enabled)

// BEGIN FUNCTIONS
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

int open_port(string port, int baud_code) {					// open a serial port
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
	options.c_lflag = ICANON;								// canonical mode
	options.c_oflag &= ~OPOST;								// raw output
	tcsetattr(iface, TCSANOW, &options);					// set the new options for the port
	fcntl(iface, F_SETFL, 0);								// set port to blocking reads
	return iface;											// return the file number
}	// END OF 'open_port'

void init(int argc, char* argv[]) {		// read config, set up serial ports, etc
	string configfile = "/etc/aprstoolkit.conf";

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
				if (strcmp(optarg, "tnc") == 0) tnc_debug = true;
				break;
			case '?':		// can't understand what the user wants from us, let's set them straight
				fprintf(stderr, "Usage: aprstoolkit [-v] [-c CONFIGFILE]\n\n");
				fprintf(stderr, "Options:\n -v\tbe verbose\n -c\tspecify config file\n -?\tshow this help\n");
				exit (EXIT_FAILURE);
				break;
			}
	}

	if (verbose) printf("APRS Toolkit %s\n\n", VERSION);

// CONFIG FILE PARSING

	INIReader readconfig(configfile);	// read the config ini

	if (readconfig.ParseError() < 0) {	// if we couldn't parse the config
		fprintf(stderr, "Error loading %s\n", configfile.c_str());
		exit (EXIT_FAILURE);
	}

	if (verbose) printf("Using config file %s\n", configfile.c_str());

	mycall = readconfig.Get("station", "mycall", "N0CALL");
	bool gps_enable = readconfig.GetBoolean("gps", "enable", false);
	string kiss_port = readconfig.Get("tnc", "port", "/dev/ttyS0");
	int kiss_baud = readconfig.GetInteger("tnc", "baud", 9600);
	string gps_port  = readconfig.Get("gps", "port", "/dev/ttyS1");
	int gps_baud = readconfig.GetInteger("gps", "baud", 4800);

// OPEN KISS INTERFACE

	// no 'if' here, since this would be pointless without a TNC

	int baud_code = get_baud(kiss_baud);

	if (baud_code == -1) {		// get_baud says that's an invalid baud rate
		fprintf(stderr, "Invalid KISS baud rate %i\n", kiss_baud);
		exit (EXIT_FAILURE);
	}

	kiss_iface = open_port(kiss_port, baud_code);

	if (kiss_iface == -1) {		// couldn't open the serial port...
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

		gps_iface = open_port(gps_port, baud_code);

		if (gps_iface == -1) {
			fprintf(stderr, "Could not open GPS port %s\n", gps_port.c_str());
			exit (EXIT_FAILURE);
		}

		if (verbose) printf("Successfully opened GPS port %s at %i baud\n", gps_port.c_str(), gps_baud);
	}

}	// END OF 'init'

void* gps_thread(void*) {		// thread to listen to the incoming NMEA stream and update our position and time
	string buff = "";
	char * data = new char[1];

	while (true) {
		read(gps_iface, data, 1);
		if (data[0] == '\n') {
			if (buff.length() > 0) {
				if (gps_debug) printf("GPS_IN: %s\n", buff.c_str());
				if (buff.compare(0, 6, "$GPRMC") == 0) {
					string params[12];
					int current = 7;
					int next;
					for (int a=0; a<12; a++) {
						next = buff.find_first_of(',', current);
						params[a] = buff.substr(current, next - current);
						current = next + 1;
					}
					if (params[1].compare("A") == 0) {
						gps_time->tm_hour = atoi(params[0].substr(0,2).c_str());
						gps_time->tm_min = atoi(params[0].substr(2,2).c_str());
						gps_time->tm_sec = atoi(params[0].substr(4,2).c_str());
						gps_time->tm_mday = atoi(params[8].substr(0,2).c_str());
						gps_time->tm_mon = atoi(params[8].substr(2,2).c_str()) - 1;		// tm_mon is 0-11
						gps_time->tm_year = atoi(params[8].substr(4,2).c_str()) + 100; 	// tm_year is "years since 1900"
						pos_lat = atof(params[2].c_str());
						if (params[3].compare("S") == 0) pos_lat = pos_lat * -1;
						pos_long = atof(params[4].c_str());
						if (params[5].compare("E") == 0) pos_long = pos_long * -1;
						if (gps_debug) printf("GPS_DEBUG: looks like %f %f %s", pos_lat, pos_long, asctime(gps_time));
					} else if (gps_debug) {
						printf("GPS-DEBUG: data invalid.\n");
					}
				}
				buff = "";
			}
		} else {
			buff.append(1, data[0]);
		}
	}
	return 0;
}

int main(int argc, char* argv[]) {

	init(argc, argv);

	if (gps_iface > 0) {
		pthread_t gps_t;
		pthread_create(&gps_t, NULL, &gps_thread, NULL);
	}

	while(true) {
		sleep(600);
	}

	if (verbose) printf("Closing TNC interface\n");
	close(kiss_iface);
	if (verbose) printf("Closing GPS interface\n");
	close(gps_iface);

	return 0;

}	// END OF 'main'
