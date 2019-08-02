#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __PSK_INC
#define __PSK_INC

// INCLUDES
#include "gpio.h"

namespace psk
{
    // VARS
    extern gpio::Pin* pttPin;

    // CLASSES
    class SampleGenerator {
        vector<int8_t> sine;
        vector<float> cosine;
        unsigned int samples;
        unsigned int pos;
        char phase;
        unsigned int center;
    public:
        void init(unsigned int, unsigned char, unsigned int, unsigned char, float);
        void swap_phase();
        int8_t get_next();
        int8_t get_next_cos(unsigned int);
        unsigned int samples_per_baud;
        unsigned int samples_per_seg;
    };

    // FUNCTIONS
    void send_aprs(unsigned int, unsigned char, gpio::Pin*, const char*, unsigned char, const char*, unsigned char, const char*);
    void send_preamble(SampleGenerator& sine, FILE *aplay, float baud);
    void send_char(char c, SampleGenerator& sine, FILE *aplay);
    void send_postamble(SampleGenerator& sine, FILE *aplay, float baud);
}

#endif