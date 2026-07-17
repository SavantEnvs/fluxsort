/* Fuzz harness for fluxsort (ported from the original fuzz/fluxsort-fuzz.c).
 *
 * Fixes over the original: bounded input copy (the original memcpy'd an
 * unbounded size into a fixed 16000-byte global) and a valid comparator
 * (the original returned uint8_t, which is not a total order). Also sorts
 * the input as an int array to exercise the 32-bit primitive paths.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fluxsort.h"

static int cmp_byte(const void *a, const void *b)
{
	return (int) *(const unsigned char *) a - (int) *(const unsigned char *) b;
}

static int cmp_int(const void *a, const void *b)
{
	int x = *(const int *) a, y = *(const int *) b;

	return (x > y) - (x < y);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size == 0 || size > 65536)
	{
		return 0;
	}

	unsigned char *bytes = (unsigned char *) malloc(size);

	if (bytes == NULL)
	{
		return 0;
	}
	memcpy(bytes, data, size);
	fluxsort(bytes, size, sizeof(unsigned char), cmp_byte);
	free(bytes);

	size_t nmemb = size / sizeof(int);

	if (nmemb)
	{
		int *ints = (int *) malloc(nmemb * sizeof(int));

		if (ints == NULL)
		{
			return 0;
		}
		memcpy(ints, data, nmemb * sizeof(int));
		fluxsort(ints, nmemb, sizeof(int), cmp_int);
		free(ints);
	}
	return 0;
}
