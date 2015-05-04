#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __INIT_INC
#define __INIT_INC
// INCLUDES
#include "INIReader.h"
#include <termios.h>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include "version.h"
#include "hamlib.h"
#include "stringfuncs.h"

// FUNCTIONS
int get_baud(int);
int open_port(string, string, int, bool, bool = false);
void init(int, char**);
#endif
