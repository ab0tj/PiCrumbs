#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

class config{
	string mycall;
};

void showusage() {
	fprintf(stderr, "Usage: aprstoolkit [-c CONFIGFILE]\n");
	exit (EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
	string configfile = "/etc/aprstoolkit.conf";
	
	if (argc > 1) {		// user entered command line arguments
		int c;
		opterr = 0;
		while ((c = getopt (argc, argv, "c:")) != -1)		// loop through all command line args
			switch (c) {
			case 'c':		// user specified a config file
				configfile = optarg;
				break;
			case '?':
				showusage();
				break;
			}
	}

	INIReader readconfig(configfile);	// read the config ini

	if (readconfig.ParseError < 0) {
		fprintf(stderr, "Error loading %s", configfile);
		exit (EXIT_FAILURE);
	}

	config::mycall = readconfig.Get("station", "mycall", "N0CALL");
	
}
