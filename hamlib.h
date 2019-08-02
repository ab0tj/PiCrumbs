#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __HAMLIB_INC
#define __HAMLIB_INC

// INCLUDES
#include <hamlib/rigclass.h>

// FUNCTIONS
bool tune_radio(freq_t, rmode_t);
freq_t get_radio_freq();
rmode_t get_radio_mode();
void hamlib_close();

// STRUCTS
struct HamlibStruct
{
    Rig* radio;								// radio control interface reference
    bool enabled;						// is radio control enabled?
};
#endif
