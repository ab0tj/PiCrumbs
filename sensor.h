#ifndef __SENSOR_H
#define __SENSOR_H

#include <string>

using namespace std;

namespace sensor
{
    class Adc
    {
        private:
            string fileName;				// file to get ADC value from
            float scale;					// ADC scaling value

        public:
            Adc(string& f, float s): fileName(f), scale(s) {};
            float read(int scale);
    };

    class Temp
    {
        private:
            string fileName;				// file to get temperature info from
            char unit;					    // temperature units: C, F, or K

        public:
            unsigned int precision;
            Temp(string& f, char u);
            float read();
            inline char get_unit() { return unit; };
    };
}

#endif