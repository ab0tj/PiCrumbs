#include "console.h"
#include <sstream>
#include <ctime>
#include "version.h"
#include "beacon.h"
#include <unistd.h>
#include "stringfuncs.h"
#include "hamlib.h"

ConsoleStruct console;
extern BeaconStruct beacon;

int console_print(string s) {
	return write(console.iface, s.c_str(), s.length());
}

string build_prompt() {
	stringstream ss;
	ss << beacon.mycall;
	if (beacon.myssid > 0) ss << '-' << (int)beacon.myssid;
	ss << "> ";
	return ss.str();
}

void show_pathstats(bool align) {
	if (align) console_print("\x1B[8;0H");
	console_print("Path: Success/Attempts (Success%)\r\n");

	stringstream buff_out;
	
	for (unsigned int i=0; i < beacon.aprs_paths.size(); i++) {		// loop thru all paths
		aprspath* path = &beacon.aprs_paths[i];
		buff_out << path->name << ": " << path->success;
		
		if (path->attempt > 0) {
			buff_out << '/' << path->attempt;
			buff_out << " (" << (int)(((float)path->success / (float)path->attempt) * 100) << "%)";
		}
		
		console_print("\x1B[K" + buff_out.str() + "\r\n");
		
		buff_out.str(string());			// clear output buffer
		buff_out.clear();
	}
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
		
		read(console.iface, data, 1);							// wait for the first char to come in the serial port
		
		while(data[0] != '\n' && data[0] != '\r') {				// keep reading until the user hits enter
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
			read(console.iface, data, 1);						// read another char
		}
		
		console_print("\r\n");
		buff_in >> param;	// get command from buffer

		if ((param.compare("help") == 0) || (param.compare("?") == 0)) {
			console_print("Available commands:\r\n\n");
			console_print("bcnnow: Send a beacon now.\r\n");
			console_print("mycall <new call>: Set or get current callsign.\r\n");
			console_print("comment <new comment>: Set or get current beacon comment.\r\n");
			console_print("symbol <c or tc>: Set or get current symbol table, character.\r\n");
			console_print("stats: Get path usage statistics.\r\n");
			console_print("info: Show tracker info.\r\n\r\n");
		}
		
		else if (param.compare("bcnnow") == 0) {
			console_print("Sending beacon...\r\n");
			
			int path = sendBeacon();
			if (path == -1) {
				console_print("Beacon path selection failed.\r\n");
			} else {
				buff_out << "Beacon successfully sent on path " << path + 1 << '.';
				console_print(buff_out.str() + "\r\n");
			}
			if (path != 0) tune_radio(&beacon.aprs_paths[0]);	// retune if necessary
		}
		
		else if (param.compare("mycall") == 0) {
			param = "";
			buff_in >> param;
			if (param.compare("")) {	// compare returns 0 on match
				string temp_call = get_call(param);
				unsigned char temp_ssid = get_ssid(param);
				if ((temp_call.length() > 6) || ((temp_ssid < 0) || (temp_ssid > 15))) {
					console_print("Error: Invalid callsign or ssid.\r\n");
				} else {
					beacon.mycall = temp_call;
					beacon.myssid = temp_ssid;
					console_print("OK\r\n");
					prompt = build_prompt();
				}
			} else {		// user is just asking
				buff_out << "mycall: " << beacon.mycall;
				if (beacon.myssid != 0) buff_out << '-' << (int)beacon.myssid;
				console_print(buff_out.str() + "\r\n");
			}
		}
		
		else if (param.compare("ping") == 0) {
			console_print("pong: Shall we play a game?\r\n");
		}
		
		else if (param.compare("symbol") == 0) {
			param = "";
			buff_in >> param;
			if (param.compare("")) {	// compare returns 0 on match
				if (param.length() == 1) {
					beacon.symbol_char = param.c_str()[0];
					console_print("OK\r\n");
				} else if (param.length() == 2) {
					beacon.symbol_table = param.c_str()[0];
					beacon.symbol_char = param.c_str()[1];
					console_print("OK\r\n");
				} else {
					console_print("Error: Symbol must be one or two characters.");
				}
			} else {
				buff_out << "symbol: " << beacon.symbol_table << beacon.symbol_char;
				console_print(buff_out.str() + "\r\n");
			}
		}
		
		else if (param.compare("comment") == 0) {
			param = "";
			getline(buff_in, param);	// comment can have spaces
			if (param.compare("")) {	// compare returns 0 on match
				beacon.comment = trim(param);
				console_print("OK\r\n");
			} else {		// user is just asking
				console_print("comment: " + beacon.comment + "\r\n");
			}
		}
		
		else if (param.compare("stats") == 0) {
			show_pathstats(false);
		}
		
		else if (param.compare("info") == 0) {
			console_print("\x1B[?25l\x1B[2JPiCrumbs <" + prompt + "\r\n");
			console_print("Press any key to quit.\r\n\n");
			console_print("GPS: \r\n");
			console_print("SB:  \r\n");
			console_print("TNC: \r\n\n");
			show_pathstats(true);
			console.disp = true;
			read(console.iface, data, 1);
			console.disp = false;
			console_print("\x1B[?25h\x1B[2J");
		}
		
		else {
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
