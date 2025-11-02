#ifndef HIPHASH_H
#define HIPHASH_H

#include "kairax/types.h"

struct siphash_key {
    uint64_t key[2];
};

void siphash_keygen(struct siphash_key* key);

uint64_t siphash_4u32(const uint32_t a, const uint32_t b, const uint32_t c,
			       const uint32_t d, struct siphash_key *key);

uint64_t siphash_2u64(const uint64_t first, const uint64_t second, struct siphash_key *key);

#endif