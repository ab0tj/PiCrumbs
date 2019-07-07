#include "console.h"
#include <sstream>
#include <ctime>
#include "version.h"
#include "beacon.h"
#include <unistd.h>
#include <stdio.h>
#include "stringfuncs.h"
#include "hamlib.h"

namespace console
{
	int iface;
	bool disp;

	int conPrint(string s) {
		return write(iface, s.c_str(), s.length());
	}

	string build_prompt() {
		stringstream ss;
		ss << beacon::mycall;
		if (beacon::myssid > 0) ss << '-' << (int)beacon::myssid;
		ss << "> ";
		return ss.str();
	}

	void show_pathstats(bool align) {
		if (align) conPrint("\x1B[8;0H");
		conPrint("Path: Success/Attempts (Success%)\r\n");

		stringstream buff_out;
		
		for (unsigned int i=0; i < beacon::aprs_paths.size(); i++) {		// loop thru all paths
			beacon::aprspath* path = &beacon::aprs_paths[i];
			buff_out << path->name << ": " << path->success;
			
			if (path->attempt > 0) {
				buff_out << '/' << path->attempt;
				buff_out << " (" << (int)(((float)path->success / (float)path->attempt) * 100) << "%)";
			}
			
			conPrint("\x1B[K" + buff_out.str() + "\r\n");
			
			buff_out.str(string());			// clear output buffer
			buff_out.clear();
		}
	}

	void* thread(void*) {	// handle console interaction
		stringstream buff_in;
		stringstream buff_out;
		string param;
		const size_t buffsz = 256;
		char* data = new char[buffsz];
		
		string prompt = build_prompt();
		
		for (;;) {
			conPrint("\r\n");							// make sure we start on a new line
			conPrint(prompt);							// send prompt to the user
			read(iface, data, buffsz - 1);			// get input from user
			conPrint("\r");							// output CR for formatting
			if (data[0] < 32 || data[0] > 127) continue;	// this wasn't a command so don't bother
			
			buff_in << data;
			buff_in >> param;								// get command from buffer

			if ((param.compare("help") == 0) || (param.compare("?") == 0)) {
				conPrint("Available commands:\r\n\n");
				conPrint("bcnnow: Send a beacon now.\r\n");
				conPrint("mycall <new call>: Set or get current callsign.\r\n");
				conPrint("comment <new comment>: Set or get current beacon comment.\r\n");
				conPrint("symbol <c or tc>: Set or get current symbol table, character.\r\n");
				conPrint("stats: Get path usage statistics.\r\n");
				conPrint("info: Show tracker info.\r\n");
			}
			
			else if (param.compare("bcnnow") == 0) {
				conPrint("Sending beacon...\r\n");
				
				int path = beacon::send();
				if (path == -1) {
					conPrint("Beacon path selection failed.\r\n");
				} else {
					buff_out << "Beacon successfully sent on path " << path + 1 << '.';
					conPrint(buff_out.str() + "\r\n");
				}
				if (path != 0) tune_radio(beacon::aprs_paths[0].freq, beacon::aprs_paths[0].mode);	// retune if necessary
			}
			
			else if (param.compare("mycall") == 0) {
				param = "";
				buff_in >> param;
				if (param.compare("")) {	// compare returns 0 on match
					string temp_call = get_call(param);
					unsigned char temp_ssid = get_ssid(param);
					if ((temp_call.length() > 6) || ((temp_ssid < 0) || (temp_ssid > 15))) {
						conPrint("Error: Invalid callsign or ssid.\r\n");
					} else {
						beacon::mycall = temp_call;
						beacon::myssid = temp_ssid;
						conPrint("OK\r\n");
						prompt = build_prompt();
					}
				} else {		// user is just asking
					buff_out << "mycall: " << beacon::mycall;
					if (beacon::myssid != 0) buff_out << '-' << (int)beacon::myssid;
					conPrint(buff_out.str() + "\r\n");
				}
			}
			
			else if (param.compare("ping") == 0) {
				conPrint("pong: Shall we play a game?\r\n");
			}
			
			else if (param.compare("symbol") == 0) {
				param = "";
				buff_in >> param;
				if (param.compare("")) {	// compare returns 0 on match
					if (param.length() == 1) {
						beacon::symbol_char = param.c_str()[0];
						conPrint("OK\r\n");
					} else if (param.length() == 2) {
						beacon::symbol_table = param.c_str()[0];
						beacon::symbol_char = param.c_str()[1];
						conPrint("OK\r\n");
					} else {
						conPrint("Error: Symbol must be one or two characters.");
					}
				} else {
					buff_out << "symbol: " << beacon::symbol_table << beacon::symbol_char;
					conPrint(buff_out.str() + "\r\n");
				}
			}
			
			else if (param.compare("comment") == 0) {
				param = "";
				getline(buff_in, param);	// comment can have spaces
				if (param.compare("")) {	// compare returns 0 on match
					beacon::comment = trim(param);
					conPrint("OK\r\n");
				} else {		// user is just asking
					conPrint("comment: " + beacon::comment + "\r\n");
				}
			}
			
			else if (param.compare("stats") == 0) {
				show_pathstats(false);
			}
			
			else if (param.compare("info") == 0) {
				conPrint("\x1B[?25l\x1B[2JPiCrumbs <" + prompt + "\r\n");
				conPrint("Press any key to return to console.\r\n\n");
				conPrint("GPS: \r\n");
				conPrint("SB:  \r\n");
				conPrint("TNC: \r\n\n");
				show_pathstats(true);
				disp = true;
				read(iface, data, 1);
				disp = false;
				conPrint("\x1B[?25h\x1B[2J");
			}
			
			else {
				conPrint("Error: Unrecognized command: " + param + "\r\n");
			}
			
			buff_in.str(string());			// clear input buffer
			buff_in.clear();
			buff_out.str(string());			// clear output buffer
			buff_out.clear();
			param = string();				// clear param
			data[0] = 0;					// empty string
		}

		delete[] data;
		return 0;
	} // END OF 'console::thread'
}
