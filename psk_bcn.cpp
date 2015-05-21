#include "psk_bcn.h"
#include <sstream>
#include <cstring>
#include <stdint.h>
#include <iomanip>

void send_psk(string s) {
	
}

void send_psk_aprs(const char* source, unsigned char source_ssid, const char* destination, unsigned char destination_ssid, const char* payload) {
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
	
	send_psk("........~" + buff.str() + "~ ");
}