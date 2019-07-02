#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __PSK_INC
#define __PSK_INC

// INCLUDES
#include <vector>
#include <stdint.h>
#include <stdio.h>
#include "gpio.h"

// CLASSES
class SampleGenerator {
	vector<int8_t> sine;
	vector<float> cosine;
	unsigned int samples;
	unsigned int pos;
	char phase;
	unsigned int center;
  public:
	void init(unsigned int, unsigned char, unsigned int, unsigned char, float);
	void swap_phase();
	int8_t get_next();
	int8_t get_next_cos(unsigned int);
	unsigned int samples_per_baud;
	unsigned int samples_per_seg;
};

// FUNCTIONS
void send_psk_char(char, SampleGenerator&, FILE*);
void send_preamble(SampleGenerator&, FILE*, float);
void send_postamble(SampleGenerator&, FILE*, float);
void send_psk_aprs(unsigned int, unsigned char, GpioPin, const char*, unsigned char, const char*, unsigned char, const char*);

#endif