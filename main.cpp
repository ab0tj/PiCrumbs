#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "INIReader.cpp"
using namespace std;

string mycall;

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

	if (readconfig.ParseError() < 0) {
		fprintf(stderr, "Error loading %s\n", configfile.c_str());
		exit (EXIT_FAILURE);
	}

	mycall = readconfig.Get("station", "mycall", "N0CALL");
	
}
