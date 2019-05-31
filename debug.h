#ifndef __DEBUG_H
#define __DEBUG_H

struct DebugStruct
{
    bool verbose;				// did the user ask for verbose mode?
    bool sb;					// did the user ask for smartbeaconing info?
    bool curl;				    // let libcurl show verbose info?
    bool fh;					// did the user ask for frequency hopping info?
    bool gps;					// did the user ask for gps debug info?
    bool hl;					// did the user ask for hamlib debug info?
    bool tnc;					// did the user ask for tnc debug info?
};

#endif