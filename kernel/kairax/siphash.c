#include "kairax/siphash.h"
#include "drivers/char/random.h"

#define ROTL64(a,b) (((a)<<(b))|((a)>>(64-b)))

#define SIPHASH_PERMUTATION(a, b, c, d) ( \
	(a) += (b), (b) = ROTL64((b), 13), (b) ^= (a), (a) = ROTL64((a), 32), \
	(c) += (d), (d) = ROTL64((d), 16), (d) ^= (c), \
	(a) += (d), (d) = ROTL64((d), 21), (d) ^= (a), \
	(c) += (b), (b) = ROTL64((b), 17), (b) ^= (c), (c) = ROTL64((c), 32))

uint64_t siphash_b(int len)
{
    return ((uint64_t) len) << 56;
}

void siphash_keygen(struct siphash_key* key)
{
    uint8_t* kkey = (uint8_t*) key;
    for (int i = 0; i < 16; i ++)
    {
        kkey[i] = krand();
    }
}

uint64_t siphash_2u64(const uint64_t first, const uint64_t second, struct siphash_key *key)
{
    uint64_t v0 = 0x736f6d6570736575ULL;
    uint64_t v1 = 0x646f72616e646f6dULL;
    uint64_t v2 = 0x6c7967656e657261ULL;
    uint64_t v3 = 0x7465646279746573ULL;
    uint64_t b = siphash_b(16);
    v3 ^= key->key[1];
	v2 ^= key->key[0];
	v1 ^= key->key[1];
	v0 ^= key->key[0];
    
    v3 ^= first;

    SIPHASH_PERMUTATION(v0, v1, v2, v3);
    SIPHASH_PERMUTATION(v0, v1, v2, v3);

    v0 ^= first;
	v3 ^= second;

    SIPHASH_PERMUTATION(v0, v1, v2, v3);
    SIPHASH_PERMUTATION(v0, v1, v2, v3);

    v0 ^= second;

    //

    v3 ^= b;

    SIPHASH_PERMUTATION(v0, v1, v2, v3);
    SIPHASH_PERMUTATION(v0, v1, v2, v3);

    v0 ^= b;
    v2 ^= 0xff;

    SIPHASH_PERMUTATION(v0, v1, v2, v3);
    SIPHASH_PERMUTATION(v0, v1, v2, v3);
    SIPHASH_PERMUTATION(v0, v1, v2, v3);
    SIPHASH_PERMUTATION(v0, v1, v2, v3);

    return (v0 ^ v1) ^ (v2 ^ v3);
}

uint64_t siphash_4u32(const uint32_t a, const uint32_t b, const uint32_t c,
			       const uint32_t d, struct siphash_key *key)
{
    return siphash_2u64((uint64_t)b << 32 | a, (uint64_t)d << 32 | c, key);
}