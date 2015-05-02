#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __GPS_INC
#define __GPS_INC
// INCLUDES
#include <sys/time.h>
#include <string>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>

// FUNCTIONS
void* gps_thread(void*);
#endif
