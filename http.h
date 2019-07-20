#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __HTTP_INC
#define __HTTP_INC
// INCLUDES
#include <cstddef>
#include <vector>
#include <string>

// FUNCTIONS
size_t curlnull( char *ptr, size_t size, size_t nmemb, void *userdata);	// empty function to make libcurl happy
bool send_aprsis_http(const char*, int, const char*, int, vector<string>, vector<char>, string&, vector<bool> = vector<bool>());

// STRUCTS
struct HttpStruct
{
    bool enabled;					    // APRS-IS Enable/Disable
    string server;					// APRS-IS server name/IP
    unsigned short int port;			// APRS-IS port number
    string proxy;					// HTTP proxy to use for APRS-IS
    unsigned short int proxy_port;	// HTTP proxy port
    string user;						// APRS-IS username/callsign
    string pass;						// APRS-IS password
};

#endif
