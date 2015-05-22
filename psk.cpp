#include "psk.h"
#include "varicode.h"
#include <cmath>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <wiringPi.h>

int gcd(int a, int b) {
  int c;
  while (a != 0) {
     c = a;
	 a = b % a;
	 b = c;
  }
  return b;
}

void SampleGenerator::init(unsigned int rate, unsigned char bits, unsigned int freq, unsigned char volume, float baud) {	// precompute a sine wave scaled by the volume value to save cpu cycles later
	const double tau = 2 * M_PI;
	phase = 1;
	samples = rate / gcd(rate, freq);
	samples_per_baud = rate / baud;
	samples_per_seg = samples_per_baud / 2;
	center = (1 << (bits - 1)) - 1;
	float vol = volume / 100. * (float) center;
	
	for (unsigned int i = 0; i < samples; i++) {
		sine.push_back(sin(freq * tau * i / rate) * vol);
	}
	
	for (unsigned int i = 0; i < samples_per_baud; i++) {	 // calculate the cosine curve
		cosine.push_back((cos((float)i / samples_per_baud * tau) + 1) / 2);
	}
}

void SampleGenerator::swap_phase() {
	phase *= -1;
}

int8_t SampleGenerator::get_next() {	// return the next value in the buffer
	if (pos >= samples) pos = 0;
	return (sine[pos++] * phase + center);
}

int8_t SampleGenerator::get_next_cos(unsigned int s) {	// return the next value in the buffer, with sine curve
	if (pos >= samples) pos = 0;
	return (sine[pos++] * phase * cosine[s] + center);
}

void send_psk_char(char c, SampleGenerator& sine, FILE *aplay) {
	varicode vc = char_to_varicode(c);
		
	for (unsigned int a = vc.size; a > 0; a--) {	// loop through char
		if (vc.bits[a - 1]) {	// no phase change
			for (unsigned int s = 0; s < sine.samples_per_baud; s++) {	// loop for samples per baud
				uint8_t sample = sine.get_next();
				fputc(sample, aplay);
			}
		} else {	// phase change
			for (unsigned int s = 0; s < sine.samples_per_seg; s++) {	// loop for samples per seg
				uint8_t sample = sine.get_next_cos(s);
				fputc(sample, aplay);
			}
		
			sine.swap_phase();	// swap phase
		
			for (unsigned int s = sine.samples_per_seg; s < sine.samples_per_baud; s++) {	// loop for samples per seg, starting in the middle of the baud
				uint8_t sample = sine.get_next_cos(s);
				fputc(sample, aplay);
			}
		}
	}
}

void send_preamble(SampleGenerator& sine, FILE *aplay, float baud) {
	for (unsigned int i = 0; i < baud + 1; i++) {	// preamble
		for (unsigned int s = 0; s < sine.samples_per_seg; s++) {
			uint8_t sample = sine.get_next_cos(s);
			fputc(sample, aplay);
		}
		
		sine.swap_phase();
		
		for (unsigned int s = sine.samples_per_seg; s < sine.samples_per_baud; s++) {	// loop for samples per seg, starting in the middle of the baud
			uint8_t sample = sine.get_next_cos(s);
			fputc(sample, aplay);
		}
	}
}

void send_postamble(SampleGenerator& sine, FILE *aplay, float baud) {
	for (unsigned int s = 0; s < sine.samples_per_baud * (baud + 1); s++) {	// postamble
		uint8_t sample = sine.get_next();
		fputc(sample, aplay);
	}
}

void send_psk(float baud, unsigned int freq, unsigned char vol, unsigned char ptt_pin, const char* text) {
	SampleGenerator sine;
	
	FILE *aplay = popen("aplay -q", "w");	// open pipe to aplay
	
	sine.init(8000, 8, freq, vol, baud);

	digitalWrite(ptt_pin, 0);	// pull this line low for PTT
	
	send_preamble(sine, aplay, baud);
	
	for (unsigned int i = 0; i < strlen(text); i++) {	// send the message
		send_psk_char(text[i], sine, aplay);
	}
	
	send_postamble(sine, aplay, baud);
	
	digitalWrite(ptt_pin, 1);	// turn off PTT		TODO: is this gonna screw with DireWolf's PTT?
	
	pclose(aplay);
}

void send_psk_aprs(unsigned int freq, unsigned char vol, unsigned char ptt_pin, const char* source, unsigned char source_ssid, const char* destination, unsigned char destination_ssid, const char* payload) {
	stringstream buff;

	buff << source;
	if (source_ssid > 0) buff << source_ssid;
	buff << '>' << destination;
	if (destination_ssid > 0) buff << destination_ssid;
	buff << ':' << payload;
	
	// Compute the MODBUS RTU CRC (stole this one from http://www.ccontrolsys.com/w/How_to_Compute_the_Modbus_RTU_Message_CRC)
	uint16_t crc = 0xFFFF;
	const char* buf = buff.str().c_str();
	for (unsigned int pos = 0; pos < strlen(buf); pos++) {
		crc ^= (uint16_t)buf[pos];          // XOR byte into least sig. byte of crc
 
		for (int i = 8; i != 0; i--) {    // Loop over each bit
			if ((crc & 0x0001) != 0) {      // If the LSB is set
				crc >>= 1;                    // Shift right and XOR 0xA001
				crc ^= 0xA001;
			} else {                           // Else LSB is not set
				crc >>= 1;                    // Just shift right
			}
		}
	}
	delete buf;
	
	buff << uppercase << setfill('0') << setw(4) << hex << crc;	// append the crc value in uppercase hex
	
	send_psk(61.25, freq, vol, ptt_pin, ("........~" + buff.str() + "~ ").c_str());
}