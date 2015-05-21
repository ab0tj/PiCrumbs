#include "psk.h"
#include "varicode.h"
#include <wiringPi.h>
#include <cmath>
#include <iostream>
#include <stdint.h>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <unistd.h>

int gcd(int a, int b) {
  int c;
  while (a != 0) {
     c = a;
	 a = b % a;
	 b = c;
  }
  return b;
}
	
class Sineclass {
	vector<int8_t> sine;
	unsigned int samples;
	unsigned int pos;
  public:
	void init(unsigned int, unsigned int, float);
	int8_t get_next();
};

void Sineclass::init(unsigned int rate, unsigned int freq, float vol) {	// precompute a sine wave scaled by the volume value to save cpu cycles later
	const double tau = 2 * M_PI;
	samples = rate / gcd(rate, freq);
	
	for (unsigned int i = 0; i < samples; i++) {
		sine.push_back(sin(freq * tau * i / rate) * vol);
	}
}

int8_t Sineclass::get_next() {	// return the next value in the buffer
	if (pos >= samples) pos = 0;
	return (sine[pos++]);
}

int main(int argc, char* argv[]) {
	char phase = 1;
	const double tau = 2 * M_PI;
	const unsigned int samplerate = 8000;
	float baud = 62.5;
	unsigned int frequency = 2100;
	const unsigned char bits = 8;
	const unsigned int center = (1 << (bits - 1)) - 1;
	float volume = (float)center;
	bool gpio_ptt = false;
	unsigned char ptt_pin = 0;
	Sineclass sine;
	
	// COMMAND LINE ARGUMENT PARSING
		if (argc > 1) {		// user entered command line arguments
		int c;
		int temp;
		opterr = 0;
		while ((c = getopt (argc, argv, "m:f:v:p:")) != -1)		// loop through all command line args
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
				if (temp > 0 && temp < 101) volume = temp / 100. * (float) center;
				break;
			case 'p':
				temp = atoi(optarg);
				if (temp > 0 && temp < 255) {
					ptt_pin = temp;
					gpio_ptt = true;
				}
				break;
			case '?':		// can't understand what the user wants from us, let's set them straight
				fprintf(stderr, "Usage: psk [-m MODE] [-f FREQUENCY] [-v VOLUME]\n\n");
				fprintf(stderr, "Options:\n -m\tPSK mode: 1=PSK31, 2=PSK63, 3=PSK125 (default 2)\n");
				fprintf(stderr, " -f\tPSK audio frequency (default 2100)\n");
				fprintf(stderr, " -v\tPSK audio volume (1-100, default 100)\n");
				fprintf(stderr, " -p\tRaspberry Pi GPIO pin for PTT\n");
				fprintf(stderr, " -?\tshow this help\n");
				exit (EXIT_FAILURE);
				break;
			}
	}
	
	const unsigned int samples_per_baud = (int)(samplerate / baud);
	const unsigned int samples_per_seg = samples_per_baud / 2;
	float cosine[samples_per_baud];
	sine.init(samplerate, frequency, volume);
	
	for (unsigned int i = 0; i < samples_per_baud; i++) {	 // calculate the cosine curve
		cosine[i] = (cos((float)i / samples_per_baud * tau) + 1) / 2;
	}
	
	if (gpio_ptt) {
		wiringPiSetup();
		pinMode(ptt_pin, OUTPUT);
		digitalWrite(ptt_pin, 0);	// pull this line low for PTT
	}
	
	for (unsigned int i = 0; i < baud + 1; i++) {	// preamble
		for (unsigned int s = 0; s < samples_per_seg; s++) {
			uint8_t sample = sine.get_next() * cosine[s] * phase + center;
			cout << sample;
		}
		
		phase = -phase;
		
		for (unsigned int s = samples_per_seg; s < samples_per_baud; s++) {	// loop for samples per seg, starting in the middle of the baud
			uint8_t sample = sine.get_next() * cosine[s] * phase + center;
			cout << sample;
		}
	}
	
	char c;
	while (cin.get(c)) {	// send the message
		varicode vc = char_to_varicode(c);
		
		for (unsigned int a = vc.size; a > 0; a--) {	// loop through char
			if (vc.bits[a - 1]) {	// no phase change
				for (unsigned int s = 0; s < samples_per_baud; s++) {	// loop for samples per baud
					uint8_t sample = sine.get_next() * phase + center;
					cout << sample;
				}
			} else {	// phase change
				for (unsigned int s = 0; s < samples_per_seg; s++) {	// loop for samples per seg
					uint8_t sample = sine.get_next() * cosine[s] * phase + center;
					cout << sample;
				}
			
				phase = -phase;	// swap phase
			
				for (unsigned int s = samples_per_seg; s < samples_per_baud; s++) {	// loop for samples per seg, starting in the middle of the baud
					uint8_t sample = sine.get_next() * cosine[s] * phase + center;
					cout << sample;
				}
			}
		}
	}
	
	for (unsigned int s = 0; s < samples_per_baud * (baud + 1); s++) {	// postamble
		uint8_t sample = sine.get_next() * phase + center;
		cout << sample;
	}
	
	if (gpio_ptt) digitalWrite(ptt_pin, 1);	// turn off PTT		TODO: is this gonna screw with DireWolf's PTT?
	
	return 0;
}