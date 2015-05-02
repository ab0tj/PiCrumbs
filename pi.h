#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __PI_INC
#define __PI_INC
// INCLUDES
#include <string>
#include <cstdio>
#include <fstream>
#include <cstdlib>
#include <wiringPi.h>
#include "beacon.h"

// FUNCTIONS
bool check_gpio(int);
int get_temp();
#endif
