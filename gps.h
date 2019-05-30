#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __GPS_INC
#define __GPS_INC

struct GpsStruct
{
    bool enabled;                   // enabled by config?
    bool valid;						// should we be sending beacons?
    float lat;					    // current latitude
    float lon;					    // current longitude
    unsigned short int alt;			// current altitude in meters
    float speed;					// speed from gps, in mph
    short int hdg;					// heading from gps
};

// FUNCTIONS
void* gps_thread(void*);
#endif
