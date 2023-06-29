struct stream_chunk;
typedef struct {
    uint32 size;
    uint8 *contents;

}stream_buffer;

typedef struct stream_chunk{
  uint32 size;
  uint8 *contents;

  struct stream_chunk *next;
}stream_chunk;

typedef struct
{
  memory_area *area;
  stream_buffer buffer;

  uint32 BitCount;
  uint32 BitBuffer;
  uint32 underflow;

  stream_chunk *first;
  stream_chunk *next;
}stream_data;


inline void
stream_RefillIfEmpty(stream_data *stream)
{
  if(stream->buffer.size == 0 && stream->first)
  {
      stream_chunk *sc        = stream->first;
      stream->buffer.size     = sc->size;
      stream->buffer.contents = sc->contents;
      stream->first           = sc->next;
  }
}

#define stream_consume_data(streaminfo, type) (type *)_stream_consume_data(streaminfo, sizeof(type))
#define stream_consume_size(streaminfo, size) _stream_consume_data(streaminfo, size)
inline void *
_stream_consume_data(stream_data *stream, uint32 size)
{
  void* data = 0;

  stream_RefillIfEmpty(stream);
  if(size <= stream->buffer.size)
  {
      data = stream->buffer.contents;
      stream->buffer.size   -= size;
      stream->buffer.contents += size;
  }
  else
  {
      stream->underflow = 1;
  }

  return(data);
}

static void
stream_PushChunk(stream_data *stream, uint32 size, void *contents)
{
    stream_chunk *chunk = memory_area_push_struct(stream->area, stream_chunk); 
    chunk->size = size;
    chunk->contents = contents;
    chunk->next = 0;

    if(stream->next)
    {
       stream->next->next = chunk;
    }
    else
    {
       stream->first = chunk;
    }
    stream->next = chunk;
}

inline void
stream_reset(stream_data *stream)
{
	stream->first = 0;
	stream->next = 0;

	memory_area_reset(stream->area);
}

inline void
stream_DiscardBits(stream_data *stream, uint32 bits)
{
   stream->BitCount -= bits;
   stream->BitBuffer >>= bits;
}
static uint32
stream_PeekBits(stream_data *stream, uint32 bits)
{
    uint32 result = 0;
    while(stream->BitCount < bits && !stream->underflow)
    {
        uint32 byte = *stream_consume_data(stream, uint8);
        stream->BitBuffer |= (byte << stream->BitCount);
        stream->BitCount += 8;
    }
    if(stream->BitCount >= bits)
    {
      result = stream->BitBuffer & ((1 << bits) - 1);
    }
    else
    {
    }

    return(result);
}
static uint32
stream_ConsumeBits(stream_data *stream, uint32 bits)
{
    uint32 result = stream_PeekBits(stream, bits);

    if(stream->BitCount >= bits)
    {
        stream_DiscardBits(stream, bits);
    }

    return(result);
}
static void
stream_FlushByte(stream_data *stream)
{
    uint32 FlushCount = stream->BitCount % 8;
    stream_ConsumeBits(stream, FlushCount);
}
static stream_data
stream_Create(memory_area *area)
{
  stream_data result = {0};
  result.area = area;

  return(result);
}
static stream_data
stream_CreateFromBuffer(stream_buffer buffer)
{
  stream_data result = {0};
  result.buffer = buffer;

  return(result);
}

static stream_data
stream_create_from_memory(u32 memory_size, u8 *memory)
{
	//create buffer
	stream_buffer buffer = {0};
	buffer.size = memory_size;
	buffer.contents = memory;
	//output resulting stream
	stream_data result = {0};
	result.buffer = buffer;
	return(result);
}
#define stream_OutStruct(stream, type) (type *)stream_OutData(stream, sizeof(type))
#define stream_OutArray(stream, type, count) (type *)stream_OutData(stream, sizeof(type)*count)
static void *
stream_OutData(stream_data *stream, uint32 size)
{
    void *contents = memory_area_push_size(stream->area, size);
    stream_PushChunk(stream, size, contents);
    return(contents);
}

#define stream_push_and_copy_array(s, src, type, count) (type *)stream_PushData(s, src, sizeof(type) * (count))
static void *
stream_PushData(stream_data *stream, void *source, uint32 size)
{
    void *contents = memory_area_push_and_copy(stream->area, source, size);
    stream_PushChunk(stream, size, contents);
    return(contents);
}

static void
stream_pushf(stream_data *stream, uint8 *format, ...)
{
    if(stream && stream->area)
    {
		//make a temporary storage for the text buffer
		 temporary_area temporary_text_buffer_area = temporary_area_begin(stream->area);

		 //should the space be cleared?
         uint8 *buffer = memory_area_push_size(stream->area, 4098);
		 u32 size_of_buffer = 4098;

         va_list args;
         va_start_m(args, format);
         uint32 TextSize = format_text_list(buffer, size_of_buffer, format, args);
         va_end_m(args);

		 //end temporary area
		 temporary_area_end(&temporary_text_buffer_area);

		 //push only the needed size.
         uint8 *finalformat = memory_area_push_and_copy(stream->area, buffer, TextSize);
         stream_PushChunk(stream, TextSize, finalformat);
    }

}
