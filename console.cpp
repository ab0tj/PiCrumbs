#include "console.h"
#include <sstream>
#include "version.h"
#include <unistd.h>
#include "stringfuncs.h"

// GLOBAL VARS
extern string mycall;							// callsign we're operating under, excluding ssid
extern unsigned char myssid;					// ssid of this station (stored as a number, not ascii)
extern char symbol_table;						// which symbol table to use
extern char symbol_char;						// which symbol to use from the table

// VARS
int console_iface;					// console serial port fd

int console_print(string s) {
	return write(console_iface, s.c_str(), s.length());
}

void* console_thread(void*) {	// handle console interaction
	stringstream buff_in;
	stringstream buff_out;
	string param;
	char * data = new char[1];
	
	buff_out << mycall;		// build prompt string
	if (myssid > 0) buff_out << '-' << (int)myssid;
	buff_out << "> ";
	string prompt = buff_out.str();
	
	console_print("\r\n");			// make sure we start on a new line
	
	while(true) {
		console_print(prompt);		// send prompt to the user
		
		read(console_iface, data, 1);							// wait for the first char to come in the serial port
		while(data[0] != '\r') {								// keep reading until the user hits enter
			buff_in << data[0];
			read(console_iface, data, 1);
		}
		console_print("\n");
		buff_in >> param;											// get command from buffer
		if ((param.compare("help") == 0) || (param.compare("?") == 0)) {
			console_print("Available commands:\r\n\n");
			console_print("mycall <new call>: Set or get current callsign.\r\n");
			console_print("symbol <tc>: Set or get current symbol table, character.\r\n");
		} else if (param.compare("mycall") == 0) {
			param = "";
			buff_in >> param;
			if (param.compare("")) {	// returns 0 if a match
				string temp_call = get_call(param);
				unsigned char temp_ssid = get_ssid(param);
				if ((temp_call.length() > 6) || ((temp_ssid < 0) || (temp_ssid > 15))) {
					console_print("e002: Invalid callsign or ssid.\r\n");
				} else {
					mycall = temp_call;
					myssid = temp_ssid;
					console_print("OK\r\n");
				}
			} else {		// user is just asking
				buff_out << "mycall: " << mycall;
				if (myssid != 0) buff_out << '-' << (int)myssid;
				console_print(buff_out.str() + "\r\n");
			}
		} else if (param.compare("ping") == 0) {
			console_print("pong: Shall we play a game?\r\n");
		} else if (param.compare("symbol") == 0) {
			param = "";
			buff_in >> param;
			if (param.compare("")) {
				
			} else {
				buff_out << "symbol: " << symbol_table << symbol_char;
				console_print(buff_out.str() + "\r\n");
			}
		} else {
			console_print("e001: Unrecognized command: " + param + "\r\n");
		}
		buff_in.str(string());			// clear input buffer
		buff_in.clear();
		buff_out.str(string());			// clear output buffer
		buff_out.clear();
	}
	return 0;
} // END OF 'console_thread'