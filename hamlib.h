#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __HAMLIB_INC
#define __HAMLIB_INC

// INCLUDES
#include <rigclass.h>

// FUNCTIONS
bool tune_radio(int);
freq_t get_radio_freq();
rmode_t get_radio_mode();
bool set_radio_freq(freq_t);
bool set_radio_mode(rmode_t);
void hamlib_close();

// STRUCTS
struct HamlibStruct
{
    Rig* radio;								// radio control interface reference
    bool enabled;						// is radio control enabled?
};
#endif
