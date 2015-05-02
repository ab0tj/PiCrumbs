#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __HTTP_INC
#define __HTTP_INC
// INCLUDES
#include <curl/curl.h>
#include <vector>
#include <string>
#include <sstream>
#include "version.h"

// FUNCTIONS
size_t curlnull( char *ptr, size_t size, size_t nmemb, void *userdata);	// empty function to make libcurl happy

bool send_aprsis_http(const char*, int, const char*, int, vector<string>, vector<char>, string, vector<bool> = vector<bool>());
#endif
