#include <cstring>
#include <fstream>
#include "sensor.h"

namespace sensor
{
    Temp::Temp(string& f, char u): fileName(f), unit (u)
    {
		precision = 0;
    }

    float Temp::read()
    {
		ifstream tempfile;
		string line;
		tempfile.open(fileName.c_str());
		if (!tempfile.is_open()) 
		{
			fprintf(stderr, "Error opening temperature file: %s\n", strerror(errno));
			return -65535;		// return absurdly low value so we know it's invalid.
		}
		getline(tempfile, line);
		float val = atoi(line.c_str()) / 1000;
		if (unit == 'F') val = 1.8*val+32;
        else if (unit == 'K') val += 274.15;
		tempfile.close();
		return val;
    }

    float Adc::read(int s)
    {
        ifstream adcfile;
		string line;
		adcfile.open(fileName.c_str());
		if (!adcfile.is_open())
		{
			fprintf(stderr, "Error opening ADC file: %s\n", strerror(errno));
			return -1;		// can't have negative voltage...
		}
		getline(adcfile, line);
		float val = atoi(line.c_str());
		if (s) val *= scale;
		adcfile.close();
		return val;
    }
}