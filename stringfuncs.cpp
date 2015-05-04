#include "stringfuncs.h"

void find_and_replace(string& subject, const string& search, const string& replace) {	// find and replace in a string, thanks Czarek Tomczak
	size_t pos = 0;
	while((pos = subject.find(search, pos)) != string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
}

string get_call(string s) {
	int index = s.find_first_of("-");
	if (index == -1) {	// no ssid specified
		return s;
	} else {			// ssid was specified
		return s.substr(0,index);	// left of the dash
	}
}

unsigned char get_ssid(string s) {
	int index = s.find_first_of("-");
	if (index == -1) {	// no ssid specified
		return 0;
	} else {			// ssid was specified
		return atoi(s.substr(index+1,2).c_str());	// right of the dash
	}
}