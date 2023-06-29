typedef struct
{
    uint8 *base;
    memory_size used;
    memory_size size;
    u32 tempCount;
}memory_area;

typedef struct{

 memory_area *area;
 memory_size area_start;

}temporary_area;

typedef struct{

	u32 used;
	u32 size;
	u8 *base;

}memory_chunk;

typedef struct memory_block{
	struct memory_block *next;
	u32 total_size;
	union{
		memory_chunk chunk;
		struct{
			u32 used;
			u32 size;
			u8 *base;
		};
	};
}memory_block;

typedef struct{
	memory_block *first_block;
	memory_block *first_free;
	u32 used;
	u32 size;
	union{
		u8 *base;
		void *basev;
	};
}memory_block_main;

static memory_block_main
memory_blocks_create(u32 size, void *memory);
static memory_area
memory_block_begin(memory_block_main *block_main);
static memory_block * 
memory_block_end(memory_block_main *block_main, memory_area block_area);
static memory_block * 
memory_block_get(memory_block_main *block_main, u32 requested_size);
static void 
memory_block_free(memory_block_main *block_main, memory_block *block);

#define memory_chunk_push_struct(chunk, type) (type *)memory_chunk_push_size_(chunk, sizeof(type))
#define memory_chunk_push_array(chunk, type, count) (type *)memory_chunk_push_size_(chunk, (count)*sizeof(type))
#define memory_chunk_push_size(chunk, size) memory_chunk_push_size_(chunk, size)
#define memory_chunk_set_struct(chunk, type, source) (type *)memory_chunk_SetData(chunk, sizeof(type), source)
static void *
memory_chunk_push_size_(memory_chunk *chunk, u32 size);

#define memory_block_push_struct(block, type)        memory_chunk_push_struct(&block->chunk, type)
#define memory_block_push_array(block, type, count)  memory_chunk_push_array(&block->chunk, type, count)
#define memory_block_push_size(block, size)          memory_chunk_push_size(&block->chunk, size)
#define memory_block_set_struct(block, type, source) memory_chunk_set_struct(&block->chunk, type, source)

//general memory functions
void* memory_Copy(void *s, void* d, memory_size size)
{
	uint32 i = 0;
	while(i < size)
	{
		((u8 *)d)[i] = ((u8 *)s)[i];
		i++;
	}
    return d;

}


#define memory_dyarray_area(type) memory_dyarray_area
#define memory_dyarray(type, name) union { memory_dyarray *name; type* name ##_;}
//#define memory_dyarray(type) type *

//darray = dynamic array
//memory_drray_delete
//memory_dyrray
//memory_dyarray_
typedef struct memory_dyarray{
	union{
		u8 *base;
		void *base_v;
	};

	u32 used;
	u32 size_of_type;
	u32 size;
	u32 extra_capacity;
	u32 last_error;

	struct memory_dyarray *next;
	struct memory_dyarray_area *expandable_area;
	struct memory_dyarray *previous;
}memory_dyarray;

typedef struct memory_dyarray_area{

	union {
		struct{
            uint8 *base;
            memory_size used;
            memory_size size;
            u32 tempCount;
		};
		memory_area data;
	};
	u32 avadible_index_from_start;
	u16 zone_count;
	u16 zone_capacity;

	memory_dyarray *first;
}memory_dyarray_area;



inline void
memory_clear(void *memory, memory_size size)
{
    for(memory_size memoryIndex = 0;
        memoryIndex < size;
        memoryIndex++)
    {
        *((u8 *)memory + memoryIndex) = 0;
    }
}

#define memory_copy_array(src, dst, type, amount) (type *)memory_copy(src, dst, sizeof(type) * (amount))
inline void * 
memory_copy(void *s, void* d, memory_size size)
{
	uint32 i = 0;
	while(i < size)
	{
		((u8 *)d)[i] = ((u8 *)s)[i];
		i++;
	}
    return d;

}

static inline void *
memory_clear_and_copy(
		void *s, void *d, memory_size size)
{
	memory_clear(d, size);
	memory_copy(s, d, size);
	return(d);
}

inline void *
memory_fill(u8 data, void *s, memory_size s_size)
{
	memory_size i = 0;
	while(i < s_size)
	{
		((u8 *)s)[i] = data;
		i++;
	}

	return(s);
}

#define memory_advance_and_read_type_from(from, type, amount) (type *)memory_advance_from(from, amount)
inline void *
memory_advance_from(void *from, memory_size amount)
{
	u8 *from_u8 = (u8 *)from;
	from_u8 += amount;
	return(from_u8);
}

#define memory_shift_array_l(memory, index, count, type) memory_shift_size_amount_l(((u8 *)memory) + ((index) * sizeof(type)), sizeof(type), sizeof(type) * ((count - 1) - (index)))
#define memory_shift_array_r(memory, amount, end_index, type) memory_shift_size_amount_r(memory, sizeof(type) * (amount), sizeof(type) * (end_index))
inline void
memory_shift_size_amount_l(void *memory,
		               memory_size shift_size,
					   memory_size shift_size_total)
{
	u8 *memory_start = (u8 *)memory;
	u8 *memory_at    = memory_start + shift_size;
	memory_size size_start = 0;
	while(size_start < shift_size_total)
	{
		//copy at to start
		memory_Copy(memory_at, memory_start, shift_size);
		memory_start += shift_size;
		memory_at    += shift_size;

		size_start += shift_size;
	}
}

inline void
memory_shift_size_amount_r(void *memory,
		            memory_size shift_size,
					memory_size shift_size_total)
{
	//start at end
	u8 *memory_to = (u8 *)memory + shift_size_total;
	u8 *memory_at = memory_to - shift_size;
	while(memory_to > (u8 *)memory)
	{
		//copy chunk size from at to to
		memory_Copy(memory_at, memory_to, shift_size);
		memory_to -= shift_size;
		memory_at -= shift_size;
	}
}

static inline void
memory_shift_size_r_until(
		void *memory0,
		void *memory1,
		memory_size shift_size)	
{
	//start at end
	u8 *memory_to = (u8 *)memory1 - shift_size;
	u8 *memory_at = memory_to - shift_size;
	while(memory_at >= (u8 *)memory0 && shift_size)
	{
		//copy chunk size from at to to
		memory_Copy(memory_at, memory_to, shift_size);

		memory_size remaining = memory_at - (u8 *)memory0;
		shift_size = remaining < shift_size ? remaining : shift_size;

		memory_to -= shift_size;
		memory_at -= shift_size;

	}
}

static inline void 
memory_shift_r_until(
		void *m0,
		void *m1)
{
	u8 *to = m1;
	u8 *at = at - 1;
	while(at >= (u8 *)m0)
	{
		*to = *at;
		at--;
		to--;
	}
}

#define memory_zero_struct(memory, type) (type *)memory_zero(memory, sizeof(type))
inline void *
memory_zero(void *memory,
		    memory_size size)
{
	u32 z = 0;
	while(z < size)
	{
		((u8 *)memory)[z] = 0;
		z++;
	}
	return(memory);
}
//memory_area

inline void
memory_area_shift_size_r(memory_area *area,
		            memory_size shift_size)
{
	Assert((area->used + shift_size) < area->size);

	memory_shift_size_amount_r(
			area->base + area->used,
			shift_size,
			(area->size - shift_size));
}

inline memory_area 
memory_area_create(memory_size size, void* contents)
{
    memory_area result = {0};

    result.size = size;
    result.base = contents;
    return(result);
}
#define Align4(v) ((v + 3) & ~3)


inline uint32
memory_get_alignment_offset(void *memory, uint32 alignment)
{
	uint32 alignmentoffset = 0;
	if(alignment)
	{
		uint32 alignmentmask = alignment - 1; 

		memory_size FinalLocation = (memory_size)memory;//area->base + area->used;

		if(FinalLocation & alignmentmask)
		{
			alignmentoffset = alignment - (FinalLocation & alignmentmask);
		}
	}
	return alignmentoffset;
}

inline uint32
memory_area_get_alignment_offset(memory_area *area, uint32 alignment)
{
	uint32 alignmentoffset = 0;
	if(alignment)
	{
		uint32 alignmentmask = alignment - 1; 

		memory_size FinalLocation = (memory_size)area->base + area->used;

		if(FinalLocation & alignmentmask)
		{
			alignmentoffset = alignment - (FinalLocation & alignmentmask);
		}
	}
	return alignmentoffset;
}
#define memory_area_clear_and_push_struct(area, type) (type *)memory_area_clear_and_push(area, sizeof(type))
#define memory_area_push_struct(area, type) (type *)memory_area_push_size(area, sizeof(type))
#define memory_area_push_array(area, type, count) (type *)memory_area_push_size(area, (count)*sizeof(type))
#define memory_area_clear_and_push_array(area, type, count) (type *)memory_area_clear_and_push(area, (count)*sizeof(type))
#define memory_area_push_size(area, size) memory_area_push_size_aligned(area, size, 4)
//Align properly with memory direction.
static void *
memory_area_push_size_aligned(memory_area *area, memory_size size, uint32 alignment)
{

  uint32 alignmentoffset = memory_area_get_alignment_offset(area, alignment);
  size += alignmentoffset;
  Assert(area->used + size <= area->size);

  void *result = area->base + area->used + alignmentoffset; 
  area->used += size;

  return(result);
}

//just pushes nothing to get the next aligned pointer
static void *
memory_area_get_next_ptr(memory_area *area)
{
	//the next pointer aligned
	void *result = memory_area_push_size(area, 0);
	return(result);
}

static void *
memory_area_push_remaining(memory_area *area)
{
	memory_size remaining_size = area->used - 4;
	void *result = memory_area_push_size(area, remaining_size);

	return(result);
}

#define memory_area_PopStruct(area, type) (type *)memory_area_pop_size(area, sizeof(type))
static void *
memory_area_pop_size(memory_area *area, memory_size size)
{
	memory_size size_pop = size;
	if(size > area->used)
	{
		size = area->used;
	}
	area->used -= size_pop;
	void *result = area->base + area->used;
	return(result);
}

static void
memory_area_pop_size_from_base(memory_area *area, memory_size size)
{
	memory_size size_pop = size;
	if(size > area->used)
	{
		size = area->used;
	}
	area->used -= size_pop;

	memory_Copy(area->base + size_pop, area->base, area->used);
}

#define memory_area_push_and_copy_struct(area, type, s) (type *)memory_area_push_and_copy(area, s, sizeof(type))
#define memory_area_push_and_copy_array(area, type, s, count) (type *)memory_area_push_and_copy(area, s, sizeof(type) * count)
static void *
memory_area_push_and_copy(memory_area *area, void *source, memory_size size)
{
    uint8 *result = memory_area_push_size(area, size);
    for(int c = 0; 
            c < size;
            c++)
    {
        result[c] = ((u8 *)source)[c];
    }
    return(result);
}

#define memory_area_clear(area) _memory_area_clear(area, 0, (area)->size)
#define memory_area_clear_used(area) _memory_area_clear(area, 0, (area)->used)
#define memory_area_preserve_size_and_clear(area, preserved_size)\
_memory_area_clear((area), (preserved_size), ((area)->size) - (preserved_size))

inline void
_memory_area_clear(memory_area *area,
		           memory_size start,
		           memory_size size)
{
    for(memory_size memoryIndex = start;
        memoryIndex < size;
        memoryIndex++)
    {
        *(area->base + memoryIndex) = 0;
    }
    area->used = start;
}

#define memory_area_clear_and_pushStruct(area, type) (type *)memory_area_clear_and_push(area, sizeof(type))

#define memory_area_clear_and_push(area, size) memory_area_clear_and_push_aligned(area, size, 4)
static void *
memory_area_clear_and_push_aligned(memory_area *area, memory_size size, u32 alignment)
{
    void *result = memory_area_push_size_aligned(area, size, alignment);

    for(memory_size memoryIndex = 0;
        memoryIndex < size;
        memoryIndex++)
    {
        *((uint8 *)result + memoryIndex) = 0;
    }
    return(result);
}

inline void
memory_area_reset(memory_area *area)
{
	area->used = 0;
}


inline memory_area
memory_area_create_from(memory_area *area, memory_size size)
{
    void *AreaContents = memory_area_push_size_aligned(area, size, 16);
    memory_area result = memory_area_create(size, AreaContents);
    return(result);
}

inline memory_area
memory_area_clear_and_create_from(memory_area *area, memory_size size)
{
    void *AreaContents = memory_area_push_size_aligned(area, size, 16);
	memory_clear(AreaContents, size);
    memory_area result = memory_area_create(size, AreaContents);
    return(result);
}

inline memory_area
memory_area_create_fromRemaining(memory_area *area)
{
	memory_area copyArea = *area;
  
  return(copyArea);
}

inline void
memory_area_check(memory_area *area)
{
    Assert(area->tempCount == 0);
}

inline temporary_area
temporary_area_begin(memory_area *area)
{
    temporary_area result = {0};
    result.area      = area;
    result.area_start = area->used;
    area->tempCount++;

    return(result);
}

inline void
temporary_area_end(temporary_area *temp_area)
{
    memory_area *area = temp_area->area;
    Assert(area->used >= temp_area->area_start);

    area->used = temp_area->area_start;

    temp_area->area = 0;
    temp_area->area_start = 0;
    area->tempCount--;
}

inline void
temporary_area_keep(temporary_area *temp_area)
{
    memory_area *area = temp_area->area;
    Assert(area->used >= temp_area->area_start);

    temp_area->area = 0;
    temp_area->area_start = 0;
    area->tempCount--;
}


inline memory_chunk
memory_area_create_chunk(memory_area *area, u32 size)
{
	memory_chunk memoryChunk = {0};

	memoryChunk.base = memory_area_push_size(area, size);
	memoryChunk.size = size;

	return(memoryChunk);
}

//#define memory_chunk_PushStructZero(chunk, type) (type *)memory_chunk_ClearAndPush(chunk, sizeof(type))
#define memory_chunk_push_struct(chunk, type) (type *)memory_chunk_push_size_(chunk, sizeof(type))
#define memory_chunk_push_array(chunk, type, count) (type *)memory_chunk_push_size_(chunk, (count)*sizeof(type))
#define memory_chunk_push_size(chunk, size) memory_chunk_push_size_(chunk, size)

#define memory_chunk_set_struct(chunk, type, source) (type *)memory_chunk_SetData(chunk, sizeof(type), source)

static void *
memory_chunk_push_size_(memory_chunk *chunk, u32 size)
{
  Assert(chunk->used + size <= chunk->size);

  void *result = chunk->base + chunk->used; 
  chunk->used += size;

  return(result);
}

static void *
memory_chunk_SetData(memory_chunk *chunk, u32 size, void *source)
{
    uint8 *result = memory_chunk_push_size(chunk, size);
    for(u32 c = 0; 
            c < size;
            c++)
    {
        result[c] = ((u8 *)source)[c];
    }
    return(result);
}

inline memory_dyarray_area *
memory_dyarray_area_create(
		memory_area *area,
		u32 total_size)
		
{
	memory_dyarray_area *result = memory_area_clear_and_push_struct(
			area, memory_dyarray_area);
	memory_size size = (total_size);

	//since this unions with a memory area, one will be created
	result->base = memory_area_push_size(area, size);
	result->size = size;
	result->tempCount = 0;

	result->zone_count = 0;

	//I might make this a parameter if I need it
	result->zone_capacity = 10;
	result->first = memory_area_clear_and_push_array(
			&result->data,
			memory_dyarray,
			result->zone_capacity);
	return(result);
}

#define memory_area_dyarrays_reset memory_expandable_zones_wipe
inline void
memory_expandable_zones_wipe(
		memory_dyarray_area *main_zone)
{
	//Clear dyarrays and memory
	memory_clear(
			main_zone->base,
			main_zone->used);
	main_zone->zone_count = 0;
	main_zone->avadible_index_from_start = 0;
	//reset "used" total size on the area.
	main_zone->used = sizeof(memory_dyarray) * main_zone->zone_capacity;
}

#define memory_dyarray_create(area_ex, type, initial_size_count, extra_capacity) \
	_memory_dyarray_create(area_ex, sizeof(type), initial_size_count, extra_capacity)
#define memory_dyarray_create_safe(area_ex, dyarray, type, initial_size_count, extra_capacity) \
	do{dyarray = _memory_dyarray_create(area_ex, sizeof(type), initial_size_count, extra_capacity); type *check = dyarray ##_;} while(0)
//	do{dyarray = _memory_dyarray_create(area_ex, sizeof(type), initial_size_count, extra_capacity); Assertc(sizeof(type) == sizeof(*dyarray ##_));} while(0)
#define mdy_create memory_dyarray_create
#define mdy_create_safe memory_dyarray_create_safe

inline memory_dyarray * 
_memory_dyarray_create(
		memory_dyarray_area *area_ex,
		u32 size_of_type,
		u32 initial_size_count,
		u32 extra_capacity)
{
	Assert(size_of_type > 0);
	//expand the zones array
	if(area_ex->zone_count + 1 >= area_ex->zone_capacity)
	{
		//shift the whole used memory to the right by the capacity
		memory_size new_capacity_size = 10 * sizeof(memory_dyarray);
		//shift the whole memory and leave space at the end of the
		//dyarray headers.
		memory_size distance_base_last_array = 
			(u8 *)(area_ex->first + area_ex->zone_count) - area_ex->base;
		Assert(area_ex->used + new_capacity_size <= area_ex->size);
	//	memory_shift_size_amount_r(
	//			area_ex->first + area_ex->zone_count,
	//			new_capacity_size,
	//			area_ex->used - new_capacity_size - distance_base_last_array);
		memory_shift_size_r_until(
				area_ex->first + area_ex->zone_count,
				area_ex->base + area_ex->used + new_capacity_size,
				new_capacity_size);

		//re-point all the arrays
		u32 z = 0;
		while(z < area_ex->zone_count)
		{
			area_ex->first[z].base += new_capacity_size;
			z++;
		}

		//clear the new added zones
		memory_clear(
				area_ex->first + area_ex->zone_count,
				new_capacity_size);
		area_ex->zone_capacity += 10;

		//count the new added capacity
		area_ex->used += new_capacity_size;
	}

	memory_dyarray *new_zone = area_ex->first + area_ex->avadible_index_from_start;
	if(area_ex->avadible_index_from_start != area_ex->zone_count)
		Assert(!new_zone->base);

	new_zone->size_of_type = size_of_type;
	new_zone->expandable_area = area_ex;
	//capacity to add per expansion, put 0 for manual
	new_zone->extra_capacity = extra_capacity;
	new_zone->size = initial_size_count * size_of_type;
	new_zone->used = 0;

	//set the base
	new_zone->base = area_ex->base + area_ex->used;
	//set initial size
	memory_area_push_size(&area_ex->data, new_zone->size);


	//look for a free index, else just increase the count
	if(area_ex->avadible_index_from_start < area_ex->zone_count)
	{
		u32 z = 0;
		area_ex->avadible_index_from_start = 0;

		while((z < area_ex->zone_count) && area_ex->first[z++].base);

		if(z && z != area_ex->zone_count)
		{
			z--;
		}

		area_ex->avadible_index_from_start = z;
	}
	else
	{
	    area_ex->zone_count++;
	    area_ex->avadible_index_from_start++;
	}
	return(new_zone);
}

inline void
memory_dyarray_delete(
		memory_dyarray *zone)
{
	u32 recovered_size = zone->size;
	memory_dyarray_area *a_main = zone->expandable_area;
	u32 zone_index = (u32)(zone - a_main->first);// / sizeof(memory_dyarray);

//	Assert(recovered_size > 0);

	//re-point the arrays that come after this one
	u32 z = 0;
	while(z < a_main->zone_count)
	{
		if(a_main->first[z].base > zone->base)
		{
		    a_main->first[z].base -= recovered_size;
		}
		z++;
	}



	a_main->avadible_index_from_start = zone_index < a_main->avadible_index_from_start ?
		zone_index : a_main->avadible_index_from_start;
	if(zone_index == a_main->zone_count - 1)
	{
		a_main->zone_count--;
	}

	memory_size distance_used_zone = (zone->expandable_area->base + zone->expandable_area->used) - zone->base;
	memory_shift_size_amount_l(
			zone->base,
			recovered_size,
			distance_used_zone);

	a_main->used -= recovered_size;
	memory_clear(zone, sizeof(memory_dyarray));
}

inline void
memory_area_expandable_extend(
		memory_dyarray_area *area_ex,
		memory_size size)
{
	memory_area_push_size(&area_ex->data, size);
}

#define memory_dyarray_count(zone) (zone->used / zone->size_of_type)
#define mdy_count memory_dyarray_count

#define memory_dyarray_push_safe(zone, var) var = (md_TYPE_CHECK(zone, var), memory_dyarray_push(zone))
#define memory_dyarray_push_set_safe(zone, var) var = (var == *(zone ##_), memory_dyarray_push(zone))
#define memory_dyarray_push(zone) _memory_expandable_zone_push_size(zone, zone->size_of_type)
#define memory_dyarray_push_amount(zone, amount) _memory_expandable_zone_push_size(zone, (amount) * (zone->size_of_type))
#define memory_dyarray_clear_and_push_amount(zone, amount) memory_dyarray_clear_and_push_amount(zone, amount)
#define memory_dyarray_get_safe(zone, var, index) var = (var == zone ##_, memory_dyarray_get(zone, index))
#define memory_dyarray_base(zone, type) ((type *)zone->base)
#define memory_dyarray_base_i(zone, type, i) ((type *)zone->base)[i]
#define memory_dyarray_set_safe(zone, var, index) var = (var == *(zone ##_), (void *)((u8 *)zone->base[index * sizeof(*zone ##_)]))
#define memory_dyarray_last_safe(zone, var) var = (var == zone ##_, (void *)(zone->base + zone->used - zone->size_of_type))

#define memory_dyarray_get(zone, index) __memory_dyarray_get(zone, index) 
//helper macro
#define md_TYPE_CHECK(zone, var) (var == zone ##_)

#define mdy_push_safe memory_dyarray_push_safe
#define mdy_push memory_dyarray_push
#define mdy_push_amount memory_dyarray_push_amount
#define mdy_clear_and_push_amount memory_dyarray_clear_and_push_amount
#define mdy_get_safe memory_dyarray_get_safe
#define mdy_last_safe memory_dyarray_last_safe
#define mdy_base memory_dyarray_base
#define mdy_base_i memory_dyarray_base_i
#define mdy_set_safe memory_dyarray_set_safe

#define memory_dyarray_base_safe(zone, type, var) (sizeof(type) == sizeof(*zone ##_) ? ((type *)zone->base) : ((type *)zone->base[-1]))

inline void *
_memory_expandable_zone_push_size(
		memory_dyarray *zone,
		u32 size)
{
	//grow if overflow
	if((zone->used + size) >= zone->size)
	{
		memory_dyarray_area *a_main = zone->expandable_area;

		//expand capacity by one and add the specified extra capacity amount
		u32 new_added_size = (zone->size_of_type * zone->extra_capacity) + size;
		zone->size += new_added_size;

		memory_size distance_end_zone = (zone->base + zone->used) - zone->expandable_area->base;
		memory_area_expandable_extend(zone->expandable_area, new_added_size);
		//shift the whole memory from this point
		memory_shift_size_amount_r(
				zone->base + zone->used,
				new_added_size,
				zone->expandable_area->size - distance_end_zone - new_added_size);

		//re-point all the arrays that come after this one
		u32 zone_index = 0;
		while(zone_index < a_main->zone_count)
		{
			memory_dyarray *ezo = a_main->first + zone_index++;
			if(ezo->base > zone->base)
			{
			    ezo->base += new_added_size;
			}
		}
	}

	void *result = zone->base + zone->used;
	zone->used += size;

	Assert(zone->used < zone->size);

	return(result);
}

inline void *
memory_dyarray_clear_and_push_amount(
		memory_dyarray *zone,
		u32 count)
{
	u32 size = zone->size_of_type * count;
	u8 *result = _memory_expandable_zone_push_size(zone, size);
	memory_clear(result, size);

	return(result);
}

inline void *
memory_dyarray_clear_and_push(
		memory_dyarray *zone)
{
	u32 size = zone->size_of_type;
	Assert(size > 0);

	u8 *result = _memory_expandable_zone_push_size(zone, size);
	memory_clear(result, size);

	return(result);
}

static void
memory_dyarray_clear_all(
		memory_dyarray *dyarray)
{
	memory_clear(dyarray->base, dyarray->size);
}

#define get_test(var, un) var = (var == un.two, un.one)
inline void *
__memory_dyarray_get(
		memory_dyarray *zone,
		u32 index)
{
	void *result = (zone->base + (zone->size_of_type * index));

	return(result);
}

inline void *
memory_dyarray_push_at(
		memory_dyarray *zone,
		u32 index)
{
	void *result = 0;
	if(index < (zone->used / zone->size_of_type))
	{
		memory_dyarray_push(zone);
		//shift the memory from where the added item should start at
		memory_shift_size_amount_r(
				zone->base + zone->used,
				zone->size_of_type,
				zone->size - zone->size_of_type);

		result = zone->base + (zone->size_of_type * index);

	}
	return(result);
}

inline u32
memory_dyarray_remove_at(
		memory_dyarray *zone,
		u32 index)
{
	u32 success = 0;
	u32 size = zone->size_of_type;
	u32 count = memory_dyarray_count(zone);

	if(index < count)
	{
		success = 1;
		memory_shift_size_amount_l(
				zone->base + (size * index),
				size,
				zone->used);
		zone->used -= size;
	}
	return(success);
}

static void
memory_dyarray_remove_amount(
		memory_dyarray *zone,
		u32 amount)
{
	u32 removed_size = (amount * zone->size_of_type);
	removed_size = zone->used < removed_size ? zone->used : removed_size;
	zone->used -= removed_size;
}

static void
memory_dyarray_set_count(
		memory_dyarray *zone,
		u32 new_count)
{
	u32 current_count = memory_dyarray_count(zone);
	if(new_count > current_count)
	{
		u32 added_amount = new_count - current_count;
		memory_dyarray_clear_and_push_amount(zone, added_amount);
	}
	else if(new_count < current_count)
	{
		u32 removed_amount = current_count - new_count;
		memory_dyarray_remove_amount(zone, removed_amount);
	}
}

inline void
memory_dyarray_set(
		memory_dyarray *zone,
		void *array,
		u32 array_count)
{
	zone->used = 0;

	memory_dyarray_push_amount(
			zone, array_count);

	memory_copy(
			array,
			zone->base,
			array_count * zone->size_of_type);
}
#define memory_dyarray_reset(dyarray) dyarray->used = 0

#define memory_dyarray_create_and_set(dyarray_area, type, array, count, extra_capacity) \
	_memory_dyarray_create_and_set(dyarray_area, sizeof(type), array, count, extra_capacity)
inline memory_dyarray *
_memory_dyarray_create_and_set(
		memory_dyarray_area *zones_main,
		u32 size_of_type,
		void *array,
		u32 array_count,
		u32 extra_capacity)
{
	memory_dyarray *new_zone = memory_dyarray_create(
			zones_main,
			size_of_type,
			array_count,
			extra_capacity);
	memory_dyarray_set(
			new_zone,
			array,
			array_count);

	return(new_zone);
}

static memory_area
memory_block_begin(memory_block_main *block_main)
{
	//u32 remaining_size = block_main->size - block_main->used;
	//memory_area area = memory_area_create(remaining_size, block_main->base + block_main->used);

	//return(area);
}

static memory_block * 
memory_block_end(memory_block_main *block_main, memory_area block_area)
{
	//u32 remaining_size = block_main->size - block_main->used;
	//if(block_area.used
	//memory_block *block = 0;
	//if(!block_main->first_block)
	//{

	//}
}

#if 0
static memory_block * 
memory_block_get(memory_block_main *block_main, u32 requested_size)
{
	u32 remaining_size = block_main->size - block_main->used;
	if(remaining_size < requested_size)
	{
		return(0);
	}

	memory_block *block = 0;
	u32 size_needed = sizeof(memory_block) + requested_size;
	u32 alingment = memory_get_alingment_offset(
			block_main->base + block_main->used + size_needed);
	u32 total_used_size = alingment + size_needed;

	if(!block_main->first_block)
	{
		block = (memory_block *)block_main->base;
		memory_clear(block, sizeof(*block));

		block->size = total_used_size;
		block->base = block_main->base + sizeof(*block);
		block->used = sizeof(*block);
		block_main->first_block = block;
	}
	if(!block)
	{
		//look for blocks with enough space
		for(memory_block *c_block = block_main->first_block;
				c_block; c_block = c_block->next)
		{
			memory_block *n_block = c_block->next;
			if(c_block->free && c_block->size >= total_used_size)
			{
				block = c_block;
				block->used = sizeof(*block);
				block->base = block_main->base + block_main->used + sizeof(*block);
				block->size = total_used_size;
			}
		}
	}

	return(block);
}
#else
static memory_block * 
memory_block_get(memory_block_main *block_main, u32 requested_size)
{
	u32 remaining_size = block_main->size - block_main->used;
	if(remaining_size < requested_size)
	{
		return(0);
	}

	memory_block *block = 0;
	u32 size_needed = sizeof(memory_block) + requested_size;
	u32 total_used_size = size_needed;
	//look for free space
	memory_block *prev = 0;
	for(memory_block *fblock = block_main->first_free; fblock; fblock = fblock->next)
	{
		//use this block
		u32 spare_size = fblock->total_size - fblock->size;
		if(fblock->total_size >= requested_size)
		{

			block = fblock;
			if(fblock == block_main->first_free)
			{
				//insert free header at the end
					u32 remaining = fblock->total_size - requested_size;
					if(remaining > sizeof(memory_block))
					{
						memory_block *new_free = (memory_block *)(fblock->base + requested_size);
						memory_clear(new_free, sizeof(*new_free));
						new_free->base = (u8 *)new_free + sizeof(*new_free);
						new_free->total_size = remaining - sizeof(memory_block);
						new_free->next = fblock->next; 
						block_main->first_free = new_free;

						fblock->total_size = (u32)((u8 *)new_free - fblock->base);
					}
					else
					{
						block_main->first_free = fblock->next;
					}
			}
			else
			{
				prev->next = block->next; 

				u32 remaining = fblock->total_size - requested_size;
				if(remaining > sizeof(memory_block))
				{
					memory_block *new_free = (memory_block *)(fblock->base + requested_size);
					memory_clear(new_free, sizeof(*new_free));
					new_free->base = (u8 *)new_free + sizeof(*new_free);
					new_free->total_size = remaining - sizeof(*new_free);
					//link end free block
					fblock->total_size = (u32)((u8 *)new_free - fblock->base);
					prev->next = new_free;
				}
			}

			break;
		}
		prev = fblock;
	}
		//get rid of previous connections
		block->next = 0;
		block->size = requested_size;//size_needed;
		if(!block_main->first_block)
		{
			block_main->first_block = block;
		}
		else
		{
			prev = 0;
			//correctly connect with other blocks
			for(memory_block *c_block = block_main->first_block;
					c_block; c_block = c_block->next)
			{
				if((memory_size)c_block > (memory_size)block)
				{
					block->next = c_block;
					if(prev)
					{
						prev->next = block;
					}
					else
					{
						block_main->first_block = block;
					}

					break;
				}
				prev = c_block;
			}
			if(prev && !prev->next) prev->next = block;
		}

	return(block);
}
#endif

static void 
mem_merge_blocks(memory_block *block0, memory_block *block1, memory_block *prev)
{
	void *block_end0 = block0->base + block0->total_size;
	void *block_end1 = block1->base + block1->total_size;
	if(block_end0 < block_end1)
	{
		if(block_end0 == block1)
		{
			block0->next = block1->next;
			block0->total_size += block1->total_size + sizeof(*block1);
			if(prev) prev->next = block0;
		}
	}
}

static void 
memory_block_free(memory_block_main *block_main, memory_block *block)
{
	//position this slot correctly
	memory_block *prev = 0;
	b32 found = 0;
	if(block == block_main->first_block) 
	{
		block_main->first_block = block->next;
	}
	else
	{
		//fix connections
		for(memory_block *c_block = block_main->first_block; c_block; c_block = c_block->next)
		{
			if(c_block->next == block)
			{
				c_block->next = block->next;
				break;
			}
		}
	}
	for(memory_block *free_b = block_main->first_free; free_b; free_b = free_b->next)
	{
		if((memory_size)free_b > (memory_size)block)
		{
			found = 1;
			block->next = free_b;
			//not the first one
			if(prev)
			{
				prev->next = block;
			}
			else
			{
				block_main->first_free = block;
			}
			mem_merge_blocks(block, free_b, prev);
			break;
		}
		prev = free_b;
	}
	Assert(found);
}

static void
memory_block_cancel()
{
}

static memory_block_main
memory_blocks_create(u32 size, void *memory)
{
	memory_block_main result = {0};
	if(size > sizeof(memory_block))
	{
		result.base = memory;
		//reserve one
		result.used = sizeof(memory_block);
		result.size = size;

		memory_block *first = result.basev;
		first->size = result.size;
		first->base = result.base;
		first->total_size = result.size;
		result.first_free = first;
	}
	return(result);
}
