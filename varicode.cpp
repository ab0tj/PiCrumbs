#include "varicode.h"
#include <string>
#include <iostream>	// for debugging

varicode char_to_varicode(char c) {
	varicode vc;

	switch(c) {		// construct a bitset including the varicode character and two zeros
		case 0: // NUL
			vc.bits = bitset<12>(string("101010101100"));
			vc.size = 12;
			break;
		case 1: // SOH
			vc.bits = bitset<12>(string("101101101100"));
			vc.size = 12;
			break;
		case 2: // STX
			vc.bits = bitset<12>(string("101110110100"));
			vc.size = 12;
			break;
		case 3: // ETX
			vc.bits = bitset<12>(string("110111011100"));
			vc.size = 12;
			break;
		case 4: // EOT
			vc.bits = bitset<12>(string("101110101100"));
			vc.size = 12;
			break;
		case 5: // ENQ
			vc.bits = bitset<12>(string("110101111100"));
			vc.size = 12;
			break;
		case 6: // ACK
			vc.bits = bitset<12>(string("101110111100"));
			vc.size = 12;
			break;
		case 7: // BEL
			vc.bits = bitset<12>(string("101111110100"));
			vc.size = 12;
			break;
		case 8: // BS
			vc.bits = bitset<12>(string("101111111100"));
			vc.size = 12;
			break;
		case 9: // HT
			vc.bits = bitset<12>(string("1110111100"));
			vc.size = 10;
			break;
		case 10: // LF
			vc.bits = bitset<12>(string("1110100"));
			vc.size = 7;
			break;
		case 11: // VT
			vc.bits = bitset<12>(string("110110111100"));
			vc.size = 12;
			break;
		case 12: // FF
			vc.bits = bitset<12>(string("101101110100"));
			vc.size = 12;
			break;
		case 13: // CR
			vc.bits = bitset<12>(string("1111100"));
			vc.size = 7;
			break;
		case 14: // SO
			vc.bits = bitset<12>(string("110111010100"));
			vc.size = 12;
			break;
		case 15: // SI
			vc.bits = bitset<12>(string("111010101100"));
			vc.size = 12;
			break;
		case 16: // DLE
			vc.bits = bitset<12>(string("101111011100"));
			vc.size = 12;
			break;
		case 17: // DC1
			vc.bits = bitset<12>(string("101111010100"));
			vc.size = 12;
			break;
		case 18: // DC2
			vc.bits = bitset<12>(string("111010110100"));
			vc.size = 12;
			break;
		case 19: // DC3
			vc.bits = bitset<12>(string("111010111100"));
			vc.size = 12;
			break;
		case 20: // DC4
			vc.bits = bitset<12>(string("110101101100"));
			vc.size = 12;
			break;
		case 21: // NAK
			vc.bits = bitset<12>(string("110110101100"));
			vc.size = 12;
			break;
		case 22: // SYN
			vc.bits = bitset<12>(string("110110110100"));
			vc.size = 12;
			break;
		case 23: // ETB
			vc.bits = bitset<12>(string("110101011100"));
			vc.size = 12;
			break;
		case 24: // CAN
			vc.bits = bitset<12>(string("110111101100"));
			vc.size = 12;
			break;
		case 25: // EM
			vc.bits = bitset<12>(string("110111110100"));
			vc.size = 12;
			break;
		case 26: // SUB
			vc.bits = bitset<12>(string("111011011100"));
			vc.size = 12;
			break;
		case 27: // ESC
			vc.bits = bitset<12>(string("110101010100"));
			vc.size = 12;
			break;
		case 28: // FS
			vc.bits = bitset<12>(string("110101110100"));
			vc.size = 12;
			break;
		case 29: // GS
			vc.bits = bitset<12>(string("111011101100"));
			vc.size = 12;
			break;
		case 30: // RS
			vc.bits = bitset<12>(string("101111101100"));
			vc.size = 12;
			break;
		case 31: // US
			vc.bits = bitset<12>(string("110111111100"));
			vc.size = 12;
			break;
		case 32: // SP
			vc.bits = bitset<12>(string("100"));
			vc.size = 3;
			break;
		case 33: // !
			vc.bits = bitset<12>(string("11111111100"));
			vc.size = 11;
			break;
		case 34: // "
			vc.bits = bitset<12>(string("10101111100"));
			vc.size = 11;
			break;
		case 35: // #
			vc.bits = bitset<12>(string("11111010100"));
			vc.size = 11;
			break;
		case 36: // $
			vc.bits = bitset<12>(string("11101101100"));
			vc.size = 11;
			break;
		case 37: // %
			vc.bits = bitset<12>(string("101101010100"));
			vc.size = 12;
			break;
		case 38: // &
			vc.bits = bitset<12>(string("101011101100"));
			vc.size = 12;
			break;
		case 39: // '
			vc.bits = bitset<12>(string("10111111100"));
			vc.size = 11;
			break;
		case 40: // (
			vc.bits = bitset<12>(string("1111101100"));
			vc.size = 10;
			break;
		case 41: // )
			vc.bits = bitset<12>(string("1111011100"));
			vc.size = 10;
			break;
		case 42: // *
			vc.bits = bitset<12>(string("10110111100"));
			vc.size = 11;
			break;
		case 43: // +
			vc.bits = bitset<12>(string("11101111100"));
			vc.size = 11;
			break;
		case 44: // ,
			vc.bits = bitset<12>(string("111010100"));
			vc.size = 9;
			break;
		case 45: // -
			vc.bits = bitset<12>(string("11010100"));
			vc.size = 8;
			break;
		case 46: // .
			vc.bits = bitset<12>(string("101011100"));
			vc.size = 9;
			break;
		case 47: // /
			vc.bits = bitset<12>(string("11010111100"));
			vc.size = 11;
			break;
		case 48: // 0
			vc.bits = bitset<12>(string("1011011100"));
			vc.size = 10;
			break;
		case 49: // 1
			vc.bits = bitset<12>(string("1011110100"));
			vc.size = 10;
			break;
		case 50: // 2
			vc.bits = bitset<12>(string("1110110100"));
			vc.size = 10;
			break;
		case 51: // 3
			vc.bits = bitset<12>(string("1111111100"));
			vc.size = 10;
			break;
		case 52: // 4
			vc.bits = bitset<12>(string("10111011100"));
			vc.size = 11;
			break;
		case 53: // 5
			vc.bits = bitset<12>(string("10101101100"));
			vc.size = 11;
			break;
		case 54: // 6
			vc.bits = bitset<12>(string("10110101100"));
			vc.size = 11;
			break;
		case 55: // 7
			vc.bits = bitset<12>(string("11010110100"));
			vc.size = 11;
			break;
		case 56: // 8
			vc.bits = bitset<12>(string("11010101100"));
			vc.size = 11;
			break;
		case 57: // 9
			vc.bits = bitset<12>(string("11011011100"));
			vc.size = 11;
			break;
		case 58: // :
			vc.bits = bitset<12>(string("1111010100"));
			vc.size = 10;
			break;
		case 59: // ;
			vc.bits = bitset<12>(string("11011110100"));
			vc.size = 11;
			break;
		case 60: // <
			vc.bits = bitset<12>(string("11110110100"));
			vc.size = 11;
			break;
		case 61: // =
			vc.bits = bitset<12>(string("101010100"));
			vc.size = 9;
			break;
		case 62: // >
			vc.bits = bitset<12>(string("11101011100"));
			vc.size = 11;
			break;
		case 63: // ?
			vc.bits = bitset<12>(string("101010111100"));
			vc.size = 12;
			break;
		case 64: // @
			vc.bits = bitset<12>(string("101011110100"));
			vc.size = 12;
			break;
		case 65: // A
			vc.bits = bitset<12>(string("111110100"));
			vc.size = 9;
			break;
		case 66: // B
			vc.bits = bitset<12>(string("1110101100"));
			vc.size = 10;
			break;
		case 67: // C
			vc.bits = bitset<12>(string("1010110100"));
			vc.size = 10;
			break;
		case 68: // D
			vc.bits = bitset<12>(string("1011010100"));
			vc.size = 10;
			break;
		case 69: // E
			vc.bits = bitset<12>(string("111011100"));
			vc.size = 9;
			break;
		case 70: // F
			vc.bits = bitset<12>(string("1101101100"));
			vc.size = 10;
			break;
		case 71: // G
			vc.bits = bitset<12>(string("1111110100"));
			vc.size = 10;
			break;
		case 72: // H
			vc.bits = bitset<12>(string("10101010100"));
			vc.size = 11;
			break;
		case 73: // I
			vc.bits = bitset<12>(string("111111100"));
			vc.size = 9;
			break;
		case 74: // J
			vc.bits = bitset<12>(string("11111110100"));
			vc.size = 11;
			break;
		case 75: // K
			vc.bits = bitset<12>(string("10111110100"));
			vc.size = 11;
			break;
		case 76: // L
			vc.bits = bitset<12>(string("1101011100"));
			vc.size = 10;
			break;
		case 77: // M
			vc.bits = bitset<12>(string("1011101100"));
			vc.size = 10;
			break;
		case 78: // N
			vc.bits = bitset<12>(string("1101110100"));
			vc.size = 10;
			break;
		case 79: // O
			vc.bits = bitset<12>(string("1010101100"));
			vc.size = 10;
			break;
		case 80: // P
			vc.bits = bitset<12>(string("1101010100"));
			vc.size = 10;
			break;
		case 81: // Q
			vc.bits = bitset<12>(string("11101110100"));
			vc.size = 11;
			break;
		case 82: // R
			vc.bits = bitset<12>(string("1010111100"));
			vc.size = 10;
			break;
		case 83: // S
			vc.bits = bitset<12>(string("110111100"));
			vc.size = 9;
			break;
		case 84: // T
			vc.bits = bitset<12>(string("110110100"));
			vc.size = 9;
			break;
		case 85: // U
			vc.bits = bitset<12>(string("10101011100"));
			vc.size = 11;
			break;
		case 86: // V
			vc.bits = bitset<12>(string("11011010100"));
			vc.size = 11;
			break;
		case 87: // W
			vc.bits = bitset<12>(string("10101110100"));
			vc.size = 11;
			break;
		case 88: // X
			vc.bits = bitset<12>(string("10111010100"));
			vc.size = 11;
			break;
		case 89: // Y
			vc.bits = bitset<12>(string("10111101100"));
			vc.size = 11;
			break;
		case 90: // Z
			vc.bits = bitset<12>(string("101010110100"));
			vc.size = 12;
			break;
		case 91: // [
			vc.bits = bitset<12>(string("11111011100"));
			vc.size = 11;
			break;
		case 92: // \ 
			vc.bits = bitset<12>(string("11110111100"));
			vc.size = 11;
			break;
		case 93: // ]
			vc.bits = bitset<12>(string("11111101100"));
			vc.size = 11;
			break;
		case 94: // ^
			vc.bits = bitset<12>(string("101011111100"));
			vc.size = 12;
			break;
		case 95: // _
			vc.bits = bitset<12>(string("10110110100"));
			vc.size = 11;
			break;
		case 96: // `
			vc.bits = bitset<12>(string("101101111100"));
			vc.size = 12;
			break;
		case 97: // a
			vc.bits = bitset<12>(string("101100"));
			vc.size = 6;
			break;
		case 98: // b
			vc.bits = bitset<12>(string("101111100"));
			vc.size = 9;
			break;
		case 99: // c
			vc.bits = bitset<12>(string("10111100"));
			vc.size = 8;
			break;
		case 100: // d
			vc.bits = bitset<12>(string("10110100"));
			vc.size = 8;
			break;
		case 101: // e
			vc.bits = bitset<12>(string("1100"));
			vc.size = 4;
			break;
		case 102: // f
			vc.bits = bitset<12>(string("11110100"));
			vc.size = 8;
			break;
		case 103: // g
			vc.bits = bitset<12>(string("101101100"));
			vc.size = 9;
			break;
		case 104: // h
			vc.bits = bitset<12>(string("10101100"));
			vc.size = 8;
			break;
		case 105: // i
			vc.bits = bitset<12>(string("110100"));
			vc.size = 6;
			break;
		case 106: // j
			vc.bits = bitset<12>(string("11110101100"));
			vc.size = 11;
			break;
		case 107: // k
			vc.bits = bitset<12>(string("1011111100"));
			vc.size = 10;
			break;
		case 108: // l
			vc.bits = bitset<12>(string("1101100"));
			vc.size = 7;
			break;
		case 109: // m
			vc.bits = bitset<12>(string("11101100"));
			vc.size = 8;
			break;
		case 110: // n
			vc.bits = bitset<12>(string("111100"));
			vc.size = 6;
			break;
		case 111: // o
			vc.bits = bitset<12>(string("11100"));
			vc.size = 5;
			break;
		case 112: // p
			vc.bits = bitset<12>(string("11111100"));
			vc.size = 8;
			break;
		case 113: // q
			vc.bits = bitset<12>(string("11011111100"));
			vc.size = 11;
			break;
		case 114: // r
			vc.bits = bitset<12>(string("1010100"));
			vc.size = 7;
			break;
		case 115: // s
			vc.bits = bitset<12>(string("1011100"));
			vc.size = 7;
			break;
		case 116: // t
			vc.bits = bitset<12>(string("10100"));
			vc.size = 5;
			break;
		case 117: // u
			vc.bits = bitset<12>(string("11011100"));
			vc.size = 8;
			break;
		case 118: // v
			vc.bits = bitset<12>(string("111101100"));
			vc.size = 9;
			break;
		case 119: // w
			vc.bits = bitset<12>(string("110101100"));
			vc.size = 9;
			break;
		case 120: // x
			vc.bits = bitset<12>(string("1101111100"));
			vc.size = 10;
			break;
		case 121: // y
			vc.bits = bitset<12>(string("101110100"));
			vc.size = 9;
			break;
		case 122: // z
			vc.bits = bitset<12>(string("11101010100"));
			vc.size = 11;
			break;
		case 123: // {
			vc.bits = bitset<12>(string("101011011100"));
			vc.size = 12;
			break;
		case 124: // |
			vc.bits = bitset<12>(string("11011101100"));
			vc.size = 11;
			break;
		case 125: // }
			vc.bits = bitset<12>(string("101011010100"));
			vc.size = 12;
			break;
		case 126: // ~
			vc.bits = bitset<12>(string("101101011100"));
			vc.size = 12;
			break;
		case 127: // DEL
			vc.bits = bitset<12>(string("111011010100"));
			vc.size = 12;
			break;
		default: // undefined
			vc.size = 0;
			break;
	}
	
	return vc;
}