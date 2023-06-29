#define hash_a (u32)2654435769
#define hash_w 32
#define hash_m 3

static inline u32
hash_u32(u32 h, u32 seed) {
	h ^= h >> 16;
	h += seed;
    h *= 0x3243f6a9U;
    h ^= h >> 16;
	return(h);
}

u32 hash_string(u8 *string)
{
  uint32 hash = 5381;
    char *str = string ;
    char c;
    while((c=*str++)) {
        hash = ((hash << 5) + hash) + c;
    }
	return(hash_u32(hash, 0));
}

inline u32
string_mi_hash(u8 *text)
{
	u8 c = 0;
	u32 i = 0;
	u32 hash = 0;
    while((c = text[i++]) != '\0')
	{
		u32 c32 = (u32)c;
		hash = c32 + (hash << 6) + (hash << 16) - hash + i;
	}
	hash = noise_u32(hash, i);

	return(hash);
}

static inline u32
hash_get_key_from_string(u8 *text, u32 limit)
{
	u32 hash = hash_string(text);
	hash %= limit;
	return(hash);
}
