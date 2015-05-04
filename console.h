#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __CONSOLE_INC
#define __CONSOLE_INC
// INCLUDES
#include <string>
#include <sstream>
#include "version.h"
#include <unistd.h>
#include "stringfuncs.h"

// FUNCTIONS
int console_print(string);
void* console_thread(void*);
#endif
