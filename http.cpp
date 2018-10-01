#include "http.h"
#include <curl/curl.h>
#include <sstream>
#include "version.h"

// GLOBAL VARS
extern bool curl_debug;
extern bool tnc_debug;
extern bool fh_debug;

// LOCAL VARS
bool aprsis_enable;					// APRS-IS Enable/Disable
string aprsis_server;					// APRS-IS server name/IP
unsigned short int aprsis_port;			// APRS-IS port number
string aprsis_proxy;					// HTTP proxy to use for APRS-IS
unsigned short int aprsis_proxy_port;	// HTTP proxy port
string aprsis_user;						// APRS-IS username/callsign
string aprsis_pass;						// APRS-IS password

size_t curlnull( char *ptr, size_t size, size_t nmemb, void *userdata) {        // empty function to make libcurl happy
        return nmemb;
}

bool send_aprsis_http(const char* source, int source_ssid, const char* destination, int destination_ssid, vector<string> via, vector<char>via_ssids, string payload, vector<bool>via_hbits) {	// send an APRS packet via APRS-IS
	CURL *curl;
	CURLcode res;
	struct curl_slist *headerlist = NULL;
	stringstream aprsis_url;
	stringstream buff;
	string aprsis_postdata;		// we'll build this in a string since stringstream seems to drop newlines


	if (aprsis_enable == 0) {
	    if (tnc_debug || curl_debug) {
		printf("APRS-IS disabled.  Not sending.\n");
	    }
	    return 1;
	}

	// first, build the TNC2 packet
	buff << "user " << aprsis_user << " pass " << aprsis_pass << " vers PiCrumbs " << VERSION;
	aprsis_postdata = buff.str();
	aprsis_postdata.append("\n");
	buff.str(string());	// clear buff and add source
	buff << source;
	if (source_ssid != 0) buff << '-' << source_ssid;
	buff << '>' << destination;
	if (destination_ssid != 0) buff << '-' << destination_ssid;
	buff << ",TCPIP*";

	if (via.size() != 0) {
		if (via_hbits.size() == 0) {							// via_hbits was not specified, fill it with zeros
			for (unsigned int i=0; i<via.size(); i++) {
				via_hbits.push_back(false);
			}
		}

		for (unsigned int i=0; i<via.size(); i++) {
			buff << ',' << via[i];
			if (via_ssids[i] != 0) buff << '-' << via_ssids[i];
			if (via_hbits[i]) buff << '*';
		}
	}

	buff << ':' << payload;
	aprsis_postdata.append(buff.str());
	aprsis_postdata.append("\n");

	// then use libcurl to squirt it out to aprs-is
	curl = curl_easy_init();
	if (curl_debug) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, *curlnull);
	headerlist = curl_slist_append(headerlist, "Accept-Type: text/plain");
	headerlist = curl_slist_append(headerlist, "Content-Type: application/octet-stream");
	aprsis_url << "http://" << aprsis_server << ':' << aprsis_port;
	curl_easy_setopt(curl, CURLOPT_URL, aprsis_url.str().c_str());
	curl_easy_setopt(curl, CURLOPT_PROXY, aprsis_proxy.c_str());
	curl_easy_setopt(curl, CURLOPT_PROXYPORT, aprsis_proxy_port);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, aprsis_postdata.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,aprsis_postdata.length());

	res = curl_easy_perform(curl);

	if (tnc_debug) {
		printf("TNC_OUT(is): %s\n", buff.str().c_str());
	}

	// a little cleanup...
	curl_easy_cleanup(curl);
	curl_slist_free_all(headerlist);

	if (fh_debug && res != CURLE_OK) printf("APRS-IS upload failed. (%i: %s)\n", res, curl_easy_strerror(res));

	return (res == CURLE_OK);
}	// END OF 'send_aprsis_http'
