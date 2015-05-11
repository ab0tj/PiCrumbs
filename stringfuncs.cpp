#include "stringfuncs.h"
#include <cstdlib>
#include <locale>
#include <algorithm>
#include <sstream>

void find_and_replace(string& subject, const string& search, const string& replace) {	// find and replace in a string, thanks Czarek Tomczak
	size_t pos = 0;
	while((pos = subject.find(search, pos)) != string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
}

string get_call(string s) {
	int index = s.find_first_of("-");
	if (index != -1) s = s.substr(0,index);	// ssid was specified
	transform(s.begin(), s.end(), s.begin(), ::toupper);	// make sure we're returning the call in all caps
	return s;
}

unsigned char get_ssid(string s) {
	int index = s.find_first_of("-");
	if (index == -1) {	// no ssid specified
		return 0;
	} else {			// ssid was specified
		return atoi(s.substr(index+1,2).c_str());	// right of the dash
	}
}

bool isNotPrintable (char c) 
{  
    return !isprint(c);   
} 

string StripNonAscii(string s) 
{ 
    s.erase(remove_if(s.begin(),s.end(), isNotPrintable), s.end());
	return s;
}

string secs_to_str(int in) {
	stringstream buff;
	unsigned int days;
	unsigned int hours;
	unsigned int mins;
	unsigned int secs;
	
	days = in / 86400;
	in %= 86400;
	hours = in / 3600;
	in %= 3600;
	mins = in / 60;
	secs = in % 60;
	
	if (days > 0) buff << days << "d ";
	if (hours > 0) buff << hours << "h ";
	if (mins > 0) buff << mins << "m ";
	if (secs > 0) buff << secs << 's';
	
	return buff.str();
}