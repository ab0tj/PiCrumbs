#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __CONSOLE_INC
#define __CONSOLE_INC
// INCLUDES
#include <string>

// FUNCTIONS
int console_print(string);
void* console_thread(void*);
void show_pathstats(bool);

// STRUCTS
struct ConsoleStruct
{
    int iface;				// console serial port fd
    bool disp;				// print smartbeaconing params to console
};
#endif
