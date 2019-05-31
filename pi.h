#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __PI_INC
#define __PI_INC

#include <string>

// FUNCTIONS
bool check_gpio(int);
int get_temp();
void init_gpio();

//STRUCTS
struct PiStruct
{
    string temp_file;						// file to get 1-wire temp info from, blank to disable
    bool temp_f;							// temp units: false for C, true for F
    bool gpio_enable;						// can we use gpio pins
    unsigned char gpio_hf_en;				// gpio pin for hf enable
    unsigned char gpio_vhf_en;				// gpio pin for vhf enable
    unsigned char gpio_psk_ptt;				// gpio pin to use for psk ptt
};
#endif
