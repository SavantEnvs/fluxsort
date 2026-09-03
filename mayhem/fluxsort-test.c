/* AUTHORED behavioral oracle for fluxsort/quadsort (upstream ships no pass/fail
 * test suite — src/bench.c is a benchmark that only prints validation errors and
 * always exits 0). This is a known-answer test: for many input patterns/sizes it
 * sorts with fluxsort / quadsort and asserts the result equals a qsort-derived
 * reference, plus a stability check (fluxsort and quadsort are documented stable).
 * Prints "PASSED <n> FAILED <m>" and exits non-zero iff m>0.
 *
 * SYMBOL-DRIFT NOTE: this repo is pinned at the vulnerable upstream commit UP
 * (f560ca7db4), which predates the fluxsort_size/quadsort_size "sort by reference"
 * API upstream added later. Building this oracle against current src/ (old code x
 * current toolchain would be fine, but old code x a *newer* API surface is not) at
 * UP fails with undefined-reference errors for those two symbols. Since they don't
 * exist yet at UP, the by-reference variants and their comparator helpers were
 * dropped from this oracle entirely — it still fully exercises fluxsort/quadsort,
 * which is everything the fuzz harness (mayhem/fluxsort-fuzz.c) targets anyway.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fluxsort.h"

static int passed = 0, failed = 0;

static int cmp_int(const void *a, const void *b)
{
	int x = *(const int *) a, y = *(const int *) b;

	return (x > y) - (x < y);
}

/* compare only the high 16 bits — low bits carry the original index for stability checks */
static int cmp_key(const void *a, const void *b)
{
	int x = *(const int *) a >> 16, y = *(const int *) b >> 16;

	return (x > y) - (x < y);
}

static unsigned rng_state = 0x12345678u;

static unsigned rng(void)
{
	rng_state ^= rng_state << 13;
	rng_state ^= rng_state >> 17;
	rng_state ^= rng_state << 5;
	return rng_state;
}

static void fill(int *a, int n, int pattern)
{
	int i;

	for (i = 0 ; i < n ; i++)
	{
		switch (pattern)
		{
			case 0: a[i] = (int) (rng() % 0x7fffffff) - 0x3fffffff; break; /* random */
			case 1: a[i] = i; break;                                       /* ascending */
			case 2: a[i] = n - i; break;                                   /* descending */
			case 3: a[i] = i % 16; break;                                  /* sawtooth */
			case 4: a[i] = 42; break;                                      /* all equal */
			case 5: a[i] = (int) (rng() % 4); break;                       /* few uniques */
			default: a[i] = (int) rng(); break;
		}
	}
}

typedef void SORTFUNC(void *array, size_t nmemb, size_t size, CMPFUNC *cmp);

static void check_sort(SORTFUNC *srt, const char *name, int n, int pattern)
{
	int *work = (int *) malloc((n ? n : 1) * sizeof(int));
	int *ref = (int *) malloc((n ? n : 1) * sizeof(int));

	fill(work, n, pattern);
	memcpy(ref, work, n * sizeof(int));
	qsort(ref, n, sizeof(int), cmp_int);
	srt(work, n, sizeof(int), cmp_int);

	if (memcmp(work, ref, n * sizeof(int)) == 0)
	{
		passed++;
	}
	else
	{
		failed++;
		printf("FAIL %s n=%d pattern=%d: output differs from qsort reference\n", name, n, pattern);
	}
	free(work);
	free(ref);
}

static void check_stability(SORTFUNC *srt, const char *name, int n)
{
	int *work = (int *) malloc((n ? n : 1) * sizeof(int));
	int i, ok = 1;

	for (i = 0 ; i < n ; i++)
	{
		work[i] = (int) ((rng() % 64) << 16 | (unsigned) i);
	}
	srt(work, n, sizeof(int), cmp_key);

	for (i = 1 ; i < n ; i++)
	{
		if ((work[i - 1] >> 16) > (work[i] >> 16) ||
		    ((work[i - 1] >> 16) == (work[i] >> 16) && (work[i - 1] & 0xffff) > (work[i] & 0xffff)))
		{
			ok = 0;
			break;
		}
	}

	if (ok)
	{
		passed++;
	}
	else
	{
		failed++;
		printf("FAIL %s n=%d: not stable / not sorted at index %d\n", name, n, i);
	}
	free(work);
}

int main(void)
{
	static const int sizes[] = { 0, 1, 2, 3, 7, 15, 16, 31, 63, 100, 512, 1000, 4096, 10000 };
	/* fluxsort_size/quadsort_size intentionally omitted — not yet present at UP, see file header. */
	static const struct { SORTFUNC *f; const char *name; } sorts[] =
	{
		{ fluxsort, "fluxsort" },
		{ quadsort, "quadsort" },
	};
	unsigned s, p, f;

	for (f = 0 ; f < sizeof(sorts) / sizeof(sorts[0]) ; f++)
	{
		for (s = 0 ; s < sizeof(sizes) / sizeof(sizes[0]) ; s++)
		{
			for (p = 0 ; p < 6 ; p++)
			{
				check_sort(sorts[f].f, sorts[f].name, sizes[s], (int) p);
			}
		}
		for (s = 0 ; s < sizeof(sizes) / sizeof(sizes[0]) ; s++)
		{
			check_stability(sorts[f].f, sorts[f].name, sizes[s]);
		}
	}

	printf("PASSED %d FAILED %d\n", passed, failed);
	return failed > 0;
}
