
#include "g_local.h"

int HexToInt( const char *string ){
	int		sign;
	int		value;
	int		c;

	// skip whitespace
	while ( *string <= ' ' ) {
		if ( !*string ) {
			return 0;
		}
		string++;
	}

	// check sign
	switch ( *string ) {
	case '+':
		string++;
		sign = 1;
		break;
	case '-':
		string++;
		sign = -1;
		break;
	default:
		sign = 1;
		break;
	}

	// read digits
	value = 0;
	do{
		c = *string++;
		if ( c >= '0' && c <='9' ) {
			c -= '0';
		} else if (c >= 'a' && c <= 'f') {
			c -= 'a' - 10;
		} else if (c >= 'A' && c <= 'F') {
			c -= 'A' - 10;
		} else {
			break;
		}

		value = value * 16 + c;
	}while(1);

	// not handling 10e10 notation...

	return value * sign;
}

unsigned int Checksum (char *str){
	int k;
	int l;
	int c = 0;
	int i;
	unsigned long int sum = 0xaaaaaa;

	if (!str || str[0] == 0){
		return 0;
	}
	k = strlen(str);
	l = 11 - k;

	if (l < 1) {
		l = 1;
	}
	while(l--){
		i = 0;
		while (str[i] && i < MAX_STRING_CHARS){
			//RoboPhred: serious checksum failure fix
			//c += (str[i++] + i + k) % 23;
			c += (str[i++] + i + l) % 23; 
			c %= 24;
			sum ^= (1 << c);
		}
	}
	return sum;
}

