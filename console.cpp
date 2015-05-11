#include "console.h"
#include <sstream>
#include <ctime>
#include "version.h"
#include "beacon.h"
#include <unistd.h>
#include "stringfuncs.h"
#include "hamlib.h"

// GLOBAL VARS
extern string mycall;							// callsign we're operating under, excluding ssid
extern unsigned char myssid;					// ssid of this station (stored as a number, not ascii)
extern char symbol_table;						// which symbol table to use
extern char symbol_char;						// which symbol to use from the table
extern vector<aprspath> aprs_paths;				// APRS paths to try, in order of preference

// VARS
int console_iface;					// console serial port fd

int console_print(string s) {
	return write(console_iface, s.c_str(), s.length());
}

string build_prompt() {
	stringstream ss;
	ss << mycall;
	if (myssid > 0) ss << '-' << (int)myssid;
	ss << "> ";
	return ss.str();
}

void* console_thread(void*) {	// handle console interaction
	stringstream buff_in;
	stringstream buff_out;
	string param;
	char * data = new char[1];
	
	string prompt = build_prompt();
	
	console_print("\r\n");			// make sure we start on a new line
	
	for (;;) {
		console_print(prompt);									// send prompt to the user
		
		read(console_iface, data, 1);							// wait for the first char to come in the serial port
		
		while(data[0] != '\n') {								// keep reading until the user hits enter
			if ((data[0] == '\b') || (data[0] == 0x7F)) {		// handle backspace
				long pos = buff_in.tellp();
				console_print("\b\b  \b\b");					// delete the echoed ^H or ^?
				if (pos > 0) {
					buff_in.seekp(pos - 1);						// remove the last char from the buffer
					buff_in << ' ';
					buff_in.seekp(pos - 1);
					console_print("\b \b");						// remove the last char from the terminal
				}
			} else {
				buff_in << data[0];								// this was just regular data
			}
			read(console_iface, data, 1);						// read another char
		}
		
		console_print("\r");
		buff_in >> param;	// get command from buffer
		if ((param.compare("help") == 0) || (param.compare("?") == 0)) {
			console_print("Available commands:\r\n\n");
			console_print("bcnnow: Send a beacon now.\r\n");
			console_print("mycall <new call>: Set or get current callsign.\r\n");
			console_print("symbol <c or tc>: Set or get current symbol table, character.\r\n");
			console_print("stats: Get path usage statistics");
		} else if (param.compare("bcnnow") == 0) {
			console_print("Sending beacon...\r\n");
			
			int path = beacon();
			if (path == -1) {
				console_print("Beacon path selection failed.\r\n");
			} else {
				buff_out << "Beacon successfully sent on path " << path + 1 << '.';
				console_print(buff_out.str() + "\r\n");
			}
			if (path != 0) tune_radio(0);	// retune if necessary
		} else if (param.compare("mycall") == 0) {
			param = "";
			buff_in >> param;
			if (param.compare("")) {	// compare returns 0 on match
				string temp_call = get_call(param);
				unsigned char temp_ssid = get_ssid(param);
				if ((temp_call.length() > 6) || ((temp_ssid < 0) || (temp_ssid > 15))) {
					console_print("Error: Invalid callsign or ssid.\r\n");
				} else {
					mycall = temp_call;
					myssid = temp_ssid;
					console_print("OK\r\n");
					prompt = build_prompt();
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
			if (param.compare("")) {	// compare returns 0 on match
				if (param.length() == 1) {
					symbol_char = param.c_str()[0];
					console_print("OK\r\n");
				} else if (param.length() == 2) {
					symbol_table = param.c_str()[0];
					symbol_char = param.c_str()[1];
					console_print("OK\r\n");
				} else {
					console_print("Error: Symbol must be one or two characters.");
				}
			} else {
				buff_out << "symbol: " << symbol_table << symbol_char;
				console_print(buff_out.str() + "\r\n");
			}
		} else if (param.compare("stats") == 0) {
			console_print("Path: Attempts, successes (success%) - last attempt\r\n\n");
			
			for (unsigned int i=0; i < aprs_paths.size(); i++) {		// loop thru all paths
				buff_out << i + 1 << ": " << aprs_paths[i].attempt;
				
				if (aprs_paths[i].attempt > 0) {
					buff_out << ", " << aprs_paths[i].success;
					buff_out << " (" << (int)((aprs_paths[i].success / aprs_paths[i].attempt) * 100) << "%) - ";
					buff_out << secs_to_str((int)difftime(time(NULL), aprs_paths[i].lastused));
				}
				
				console_print(buff_out.str() + "\r\n");
				
				buff_out.str(string());			// clear output buffer
				buff_out.clear();
			}
		} else {
			console_print("Error: Unrecognized command: " + param + "\r\n");
		}
		
		buff_in.str(string());			// clear input buffer
		buff_in.clear();
		buff_out.str(string());			// clear output buffer
		buff_out.clear();
		param = string();				// clear param
	}
	return 0;
} // END OF 'console_thread'