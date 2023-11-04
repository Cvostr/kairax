#include "align.h"

uint64_t align(uint64_t val, size_t alignment)
{
	if (val < alignment)
		return alignment;

	if (val % alignment != 0) {
        val += (alignment - (val % alignment));
    }

	return val;
}

uint64_t align_down(uint64_t val, size_t alignment)
{
	return val - (val % alignment);
}