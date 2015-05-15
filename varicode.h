#ifndef __NS_STD
#define __NS_STD
using namespace std;
#endif

#ifndef __VARICODE_INC
#define __VARICODE_INC
// INCLUDES
#include <bitset>

// STRUCTS
struct varicode {
	bitset<12> bits;
	unsigned char size;
};

// FUNCTIONS
varicode char_to_varicode(char);
#endif