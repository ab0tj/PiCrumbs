#include "http.h"
#include <curl/curl.h>
#include <sstream>
#include "version.h"
#include "debug.h"

HttpStruct http;

size_t curlnull( char *ptr, size_t size, size_t nmemb, void *userdata) {        // empty function to make libcurl happy
        return nmemb;
}

bool send_aprsis_http(const char* source, int source_ssid, const char* destination, int destination_ssid, vector<string> via, vector<char>via_ssids, string& payload, vector<bool>via_hbits) {	// send an APRS packet via APRS-IS
	CURL *curl;
	CURLcode res;
	struct curl_slist *headerlist = NULL;
	stringstream aprsis_url;
	stringstream buff;
	string aprsis_postdata;		// we'll build this in a string since stringstream seems to drop newlines


	if (http.enabled == 0) {
	    if (debug.tnc || debug.curl) {
		printf("APRS-IS disabled.  Not sending.\n");
	    }
	    return 1;
	}

	// first, build the TNC2 packet
	buff << "user " << http.user << " pass " << http.pass << " vers PiCrumbs " << VERSION;
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
	if (debug.curl) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, *curlnull);
	headerlist = curl_slist_append(headerlist, "Accept-Type: text/plain");
	headerlist = curl_slist_append(headerlist, "Content-Type: application/octet-stream");
	aprsis_url << "http://" << http.server << ':' << http.port;
	curl_easy_setopt(curl, CURLOPT_URL, aprsis_url.str().c_str());
	curl_easy_setopt(curl, CURLOPT_PROXY, http.proxy.c_str());
	curl_easy_setopt(curl, CURLOPT_PROXYPORT, http.proxy_port);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, aprsis_postdata.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,aprsis_postdata.length());

	res = curl_easy_perform(curl);

	if (debug.tnc) {
		printf("TNC_OUT(is): %s\n", buff.str().c_str());
	}

	// a little cleanup...
	curl_easy_cleanup(curl);
	curl_slist_free_all(headerlist);

	if (debug.fh && res != CURLE_OK) printf("APRS-IS upload failed. (%i: %s)\n", res, curl_easy_strerror(res));

	return (res == CURLE_OK);
}	// END OF 'send_aprsis_http'
