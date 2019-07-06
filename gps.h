#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __GPS_INC
#define __GPS_INC

#include "gpio.h"

namespace gps
{
    enum SpeedUnit { MPH, KPH, MPS, KT };

    struct PosStruct
    {
        bool valid;						// should we be sending beacons?
        float lat;					    // current latitude
        float lon;					    // current longitude
        unsigned short int alt;			// current altitude in meters
        float speed;					// speed from gps, in m/s
        float localSpeed;               // speed in local units
        char* spdUnitName;              // name of local speed unit
        short int hdg;					// heading from gps
    };

    extern bool enabled;
    extern gpio::Led* led;
    void* gps_thread(void*);
    PosStruct getPos();
}

#endif
