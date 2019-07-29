#include "psk.h"
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>

int main(int argc, char* argv[]) {
	const unsigned int samplerate = 8000;
	float baud = 62.5;
	unsigned int frequency = 2100;
	const unsigned char bits = 8;
	unsigned char volume = 100;
	bool gpio_ptt = false;
	unsigned char ptt_pin = 0;
	SampleGenerator sine;
	FILE* outfile;
	bool use_stdout = false;
	
	// COMMAND LINE ARGUMENT PARSING
		if (argc > 1) {		// user entered command line arguments
		int c;
		int temp;
		opterr = 0;
		while ((c = getopt (argc, argv, "sm:f:v:p:")) != -1)		// loop through all command line args
			switch (c) {
			case 'm':		// psk mode
				if (atoi(optarg) == 1) baud = 31.25;
				else if (atoi(optarg) == 2) baud = 62.5;
				else if (atoi(optarg) == 3) baud = 125;
				break;
			case 'f':		// psk carrier frequency
				temp = atoi(optarg);
				if (temp > 0 && temp < 4000) frequency = temp;
				break;
			case 'v':
				temp = atoi(optarg);
				if (temp > 0 && temp < 101) volume = temp;
				break;
			case 'p':
				temp = atoi(optarg);
				if (temp >= 0 && temp < 255) {
					ptt_pin = temp;
					gpio_ptt = true;
				}
				break;
			case 's':
				use_stdout = true;
				break;
			case '?':		// can't understand what the user wants from us, let's set them straight
				fprintf(stderr, "Usage: psk [-s] [-m MODE] [-f FREQUENCY] [-v VOLUME] [-p PTT PIN]\n\n");
				fprintf(stderr, "Options:\n -m\tPSK mode: 1=PSK31, 2=PSK63, 3=PSK125 (default 2)\n");
				fprintf(stderr, " -f\tPSK audio frequency (default 2100)\n");
				fprintf(stderr, " -v\tPSK audio volume (1-100, default 100)\n");
				fprintf(stderr, " -p\tRaspberry Pi GPIO pin for PTT\n");
				fprintf(stderr, " -s\tOutput to stdout instead of aplay\n");
				fprintf(stderr, " -?\tshow this help\n");
				exit (EXIT_FAILURE);
				break;
			}
	}
	
	sine.init(samplerate, bits, frequency, volume, baud);
	
	if (use_stdout) {
		outfile = stdout;
	} else {
		outfile = popen("aplay -q", "w");
	}
	
	if (gpio_ptt) {
		wiringPiSetup();
		pinMode(ptt_pin, OUTPUT);
		digitalWrite(ptt_pin, 0);	// pull this line low for PTT
	}
	
	send_preamble(sine, outfile, baud);
	
	char c;
	while (cin.get(c)) {	// send the message
		send_psk_char(c, sine, outfile);
	}
	
	send_postamble(sine, outfile, baud);
	
	if (gpio_ptt) digitalWrite(ptt_pin, 1);	// turn off PTT
	
	if (!use_stdout) pclose(outfile);
	
	return 0;
}