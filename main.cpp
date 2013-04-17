#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "INIReader.cpp"

using namespace std;

const char* version = "0.1";
string mycall;
bool verbose = false;
int kiss_iface;

void init(int argc, char* argv[]) {
	string configfile = "/etc/aprstoolkit.conf";

// COMMAND LINE ARGUMENT PARSING
	
	if (argc > 1) {		// user entered command line arguments
		int c;
		opterr = 0;
		while ((c = getopt (argc, argv, "vc:")) != -1)		// loop through all command line args
			switch (c) {
			case 'c':		// user specified a config file
				configfile = optarg;
				break;
			case 'v':
				verbose = true;	// user wants to hear about what's going on
				break;
			case '?':		// can't understand what the user wants from us, let's set them straight
				fprintf(stderr, "Usage: aprstoolkit [-v] [-c CONFIGFILE]\n\n");
				fprintf(stderr, "Options:\n -v\tbe verbose\n -c\tspecify config file\n -?\tshow this help\n");
				exit (EXIT_FAILURE);
				break;
			}
	}

	if (verbose) printf("APRS Toolkit %s\n\n", version);

// CONFIG FILE PARSING

	INIReader readconfig(configfile);	// read the config ini

	if (readconfig.ParseError() < 0) {	// if we couldn't parse the config
		fprintf(stderr, "Error loading %s\n", configfile.c_str());
		exit (EXIT_FAILURE);
	}

	if (verbose) printf("Using config file %s\n", configfile.c_str());

	mycall = readconfig.Get("station", "mycall", "N0CALL");
	char* kissport = readconfig.Get("tnc", "port", "/dev/ttyS0");

// OPEN KISS INTERFACE

	kiss_iface = open(kissport, 0_RDWR | 0_NOCTTY | 0_NDELAY);

	if (kiss_iface == -1) {		// couldn't open the serial port...
		fprintf(stderr, "Could not open KISS port %s\n", kissport);
		exit (EXIT_FAILURE);
	}

	fcntl(kiss_iface, F_SETFL, 0);		// set file flags

	if (verbose) printf("Successfully opened KISS port %s\n", kissport);

}	// END OF 'init'

int main(int argc, char* argv[]) {

	init(argc, argv);

}	// END OF 'main'
