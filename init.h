#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __INIT_INC
#define __INIT_INC

// INCLUDES
#include <string>

// FUNCTIONS
int get_baud(int);
int open_port(string, string, int, bool, bool = false);
void init(int, char**);
#endif
