#include "GGPOGame.h"



// UE4: allow Windows platform types to avoid naming collisions
//  this must be undone at the bottom of this file
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/prewindowsapi.h"

#define WIN32_LEAN_AND_MEAN 
#include <windows.h>

#include <timeapi.h>


/* 
 * Simple checksum function from wikipedia:
 *   http://en.wikipedia.org/wiki/Fletcher%27s_checksum
 */
int
fletcher32_checksum(short *data, size_t len)
{
	int sum1 = 0xffff, sum2 = 0xffff;

	while (len) {
		size_t tlen = len > 360 ? 360 : len;
		len -= tlen;
		do {
			sum1 += *data++;
			sum2 += sum1;
		} while (--tlen);
		sum1 = (sum1 & 0xffff) + (sum1 >> 16);
		sum2 = (sum2 & 0xffff) + (sum2 >> 16);
	}

	/* Second reduction step to reduce sums to 16 bits */
	sum1 = (sum1 & 0xffff) + (sum1 >> 16);
	sum2 = (sum2 & 0xffff) + (sum2 >> 16);
	return sum2 << 16 | sum1;
}

uint32 get_time()
{
	return timeGetTime();
}

// UE4: disallow windows platform types
//  this was enabled at the top of the file
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"