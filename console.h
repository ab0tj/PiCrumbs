#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __CONSOLE_INC
#define __CONSOLE_INC
// INCLUDES
#include <string>

namespace console
{
    int conPrint(string);
    void* thread(void*);
    void show_pathstats(bool);

    extern int iface;				// console serial port fd
    extern bool disp;				// print smartbeaconing params to console
}
#endif
