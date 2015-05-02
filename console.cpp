#include "console.h"

// VARS
int console_iface;					// console serial port fd

int console_print(string s) {
	return write(console_iface, s.c_str(), s.length());
}

void* console_thread(void*) {	// handle console interaction
	string prompt = "PiCrumbs" VERSION "> ";
	stringstream buff;
	string param;
	char * data = new char[1];
	
	while(true) {
		console_print(prompt);	// send prompt to the user
		
		read(console_iface, data, 1);							// wait for the first char to come in the serial port
		while(data[0] != '\n') {								// keep reading until the user hits enter
			buff << data[0];
			read(console_iface, data, 1);
		}
		
		buff >> param;											// get command from buffer
		if ((param == "help") || (param == "?")) {
				console_print("Available commands:\n\n");
		}
	}
} // END OF 'console_thread'