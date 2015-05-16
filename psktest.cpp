#include "psk.h"
#include "varicode.h"
#include <cmath>
#include <iostream>
#include <stdint.h>
#include <cstdlib>
#include <vector>
	
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
	samples = rate / freq;
	
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
	const float baud = 62.5;
	const unsigned int frequency = atoi(argv[1]);
	const unsigned char bits = 8;
	const unsigned int center = (1 << (bits - 1)) - 1;
	const float volume = 100 / 100. * (float)center;
	const unsigned int samples_per_baud = (int)(samplerate / baud);
	const unsigned int samples_per_seg = samples_per_baud / 2;
	float cosine[samples_per_baud];
	Sineclass sine;
	
	sine.init(samplerate, frequency, volume);
	
	for (unsigned int i = 0; i < samples_per_baud; i++) {	 // calculate the cosine curve
		cosine[i] = (cos((float)i / samples_per_baud * tau) + 1) / 2;
	}
	
	for (unsigned int i = 0; i < 32; i++) {	// preamble
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
	
	for (unsigned int s = 0; s < samples_per_baud * 32; s++) {	// postamble
		uint8_t sample = sine.get_next() * phase + center;
		cout << sample;
	}
	
	return 0;
}