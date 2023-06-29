//
//Reserved space__
//
//
//Misc/general functions
//

#if 1
#define i32_Add_OVERFLOWS(v0, v1) (((v1) > 0) && ((v0) > I32MAX - (v1)))
#define i32_Sub_OVERFLOWS(v0, v1) (((v1) < 0) && ((v0) > I32MAX + (v1)))

#define i32_Add_UNDERFLOWS(v0, v1) (((v1) < 0) && ((v0) < I32MIN - (v1)))
#define i32_Sub_UNDERFLOWS(v0, v1) (((v1) > 0) && ((v0) < I32MIN + (v1)))

#define u32_Add_OVERFLOWS(v0, v1) ((v1 > 0) && (v0) > U32MAX - (v1))
#define u32_Sub_UNDERFLOWS(v, dec) ((dec) > (v))

#define u16_Add_OVERFLOWS(v, inc) ((inc) > U16MAX - (v))
#define u16_Sub_UNDERFLOWS(v, dec) ((dec) > (v))

#define f32_Add_OVERFLOWS(v, inc) ((inc) > F32MAX - (v))
#define f32_Sub_UNDERFLOWS(v, dec) (((v) < 0) && ((dec) > ((v) + F32MAX)))

#define f32_NEARZERO(f) (f >= -1e-6f && f <= 1e-6f)
#else
#define u32_Add_OVERFLOWS(v, inc) (inc > U32MAX - (v))
#define u32_Sub_UNDERFLOWS(v, dec) ((dec) > (v))

#define u16_Add_OVERFLOWS(v, inc) (inc > U16MAX - (v))
#define u16_Sub_UNDERFLOWS(v, dec) ((dec) > (v))
#endif

#define DEFAULT_FORMAT_PRECISION 3

typedef struct{
	u8 text_buffer[256];
	u32 text_length;
}text_format_quick;

inline vec3
vertices_get_mid_point(vec3 v0,
		               vec3 v1,
					   vec3 v2,
					   vec3 v3)
{

    vec3 distance_v2_v0 = {
		v2.x - v0.x,
		v2.y - v0.y,
		v2.z - v0.z};

    
    vec3 midPoint = {  
	                    v0.x + 0.5f * (distance_v2_v0.x),
	                    v0.y + 0.5f * (distance_v2_v0.y),
	                    v0.z + 0.5f * (distance_v2_v0.z),
	};
	return(midPoint);
}


//#define point_inside_rec(mouseP, rec) MouseOverPoints(mouseP, (real32)rec.x, (real32)rec.y, (real32)(rec.x + rec.w), (real32)(rec.y + rec.h))
//#define point_inside_xywh(mouseP, pX, pY, pW, pH) MouseOverPoints(mouseP, pX, pY, pX + pW, pY + pH)

//
// General operations functions
//

inline uint32 
reverse_bits(uint32 v, uint32 bit_amount)
{
        uint32 result = 0;
     for(uint32 bitflipi = 0;
                bitflipi <= (bit_amount / 2);
                bitflipi++)
             {
               uint32 inv = (bit_amount - (bitflipi + 1));
               result |= ((v >> bitflipi) & 0x1) << inv;
               result |= ((v >> inv) & 0x1) << bitflipi;
             }
 return(result);
}


inline void
endian_swap_32(uint32 *v)
{
    uint32 _v = (*v);
#if 0
    uint8 b1 = ((_v >> 0) & 0xff);
    uint8 b2 = ((_v >> 8) & 0xff); 
    uint8 b3 = ((_v >> 16) & 0xff);
    uint8 b4 = ((_v >> 24) & 0xff);

    _v = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4 << 0);
#endif
    _v = (_v << 24) |
         ((_v & 0xff00) << 8) |
         ((_v >> 8) & 0xff00) |
         (_v >> 24);
    (*v) = _v;
}

inline void
endian_swap_16(uint16 *v)
{
    uint16 _v = (*v);
    _v = ((_v << 8) | (_v >> 8));
    (*v) = _v;
}
//
// __sorting functions
//

static void
u32_array_radix_sort_ascending(u32 *storage_f,
		                       u32 *storage_s,
					           u32 storage_count)
{
   u32 *source      = storage_f;
   u32 *destination = storage_s;

   for(int byte = 0; 
           byte <= 32;
           byte += 8)
   {

       u32 radix_displacements[256] = {0};

	   //first phase, count the number of repeating byte values
       for(u32 i = 0; 
               i < storage_count;
               i++)
       {
           u32 value  = source[i];
           u32 radix_index = (value >> byte) & 0xff; //Sorts in ascending order
           radix_displacements[radix_index]++;
       }
       //Second phase, sort and count the radix indexes.
	   //orden de menor a mayor
       u32 byte_values_total = 0;
       for(u32 RadixI = 0;
               RadixI < 256;
               RadixI++)
       {
           u32 byte_value              = radix_displacements[RadixI];
           radix_displacements[RadixI] = byte_values_total;
           byte_values_total           += byte_value;
       }
       //Last phase: sort the destination and source
       for(u32 i = 0;
               i < storage_count; 
               i++)
       {
           u32 value       = source[i];
           int radix_index = (value >> byte) & 0xff;
		   //add to the array to avoid repeated indices
		   u32 destination_index          = radix_displacements[radix_index]++;
           destination[destination_index] = source[i];
       }
       //Swap source and destination
       u32 *tempcopy = destination;
       destination = source;
       source = tempcopy;
   }

}

static inline void
u32_array_insertion_sort_ascending(u32 *array, u32 array_count)
{
    for(u32 a = 0; a < array_count; a++)
    {
        for(u32 b = 0; b < array_count; b++)
        {
            if(array[a] < array[b])
            {
               u32 value_copy_a = array[a];
               array[a]         = array[b];
               array[b]         = value_copy_a;

            }
        }
    }

}
static inline void
u32_array_insertion_sort_descending(u32 *array, u32 array_count)
{
    for(u32 a = 0; a < array_count; a++)
    {
        for(u32 b = 0; b < array_count; b++)
        {
            if(array[a] > array[b])
            {
               u32 value_copy_a = array[a];
               array[a]         = array[b];
               array[b]         = value_copy_a;

            }
        }
    }

}
//
// sorting functions__
//

//
// string functions
//
static u32
u32_from_string(u8 *text, u32 *value)
{
	u32 success = 0;
	u8 c = 0;
	u32 i = 0;
	//Make sure it's a number
	while((c = text[i]) != '\0')
	{
		if(c < '0' || c > '9')
		{
			return(0);
		}
		i++;
	}

	success = 1;
	u32 digitCount = i - 1;
	i = 0;
	c = 0;
	u32 number = 0;
	
	while((c = text[i]) != '\0')
	{
#if 1
		//Check multiply overflow
	   if(number && (10 > U32MAX / number))
	   {
		   number = U32MAX;
		   break;
	   }
#endif
	   u32 digit = c - '0';
	   number    *= 10;
	   number    += digit;
	   i++;
	}
	*value = number;
	return(success);

}

inline i32
i32_from_string(u8 *text, i32 *value)
{
	u32 unsigned_value = 0;
	u32 is_negative = text[0] == '-';

	u32 success = u32_from_string(text + is_negative, &unsigned_value);

	if(unsigned_value > I32MAX)
	{
		unsigned_value = I32MAX;
	}
	else if(unsigned_value > -I32MIN)
	{
		unsigned_value = -I32MIN;
	}
	i32 result = is_negative ? -(i32)unsigned_value : unsigned_value;
	*value = result;
	return(result);
}

static u32
f32_from_string(u8 *_text, f32 *value)
{
	u32 success = 0;
	u8 c = 0;
	u32 i = 0;
	u32 is_negative = _text[0] == '-';
	u8 *text = _text + is_negative;
	//Make sure it's a number
	while((c = text[i]) != '\0')
	{
		if((c < '0' || c > '9') && c != '.')
		{
			return(0);
		}
		i++;
	}

	success = 1;
	u32 digitCount = i - 1;
	u32 decimalCount = 0;
	i = 0;
	c = 0;
	u32 number = 0;
	
	while((c = text[i]) != '\0')
	{
		if(c == '.')
		{
			decimalCount = digitCount - i;
		}
		else
		{
	       u32 digit = c - '0';
	       number    *= 10;
	       number    += digit;
		}
	    i++;
	}

	f32 tenPow  = 10;
	f32 numberFinal = (f32)number;

	if(decimalCount)
	{
	    while(decimalCount > 1)
	    {
	    	tenPow *= 10;
	    	decimalCount--;
	    }
	    numberFinal /= tenPow;
	}
	if(is_negative)
	{
		numberFinal = -numberFinal;
	}
	*value = numberFinal;
	return(success);
}

inline u32 
char_is_letter(u8 c)
{
	return((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

inline u8
char_to_low(u8 c)
{
	if(c >= 'A' && c <= 'Z')
	{
		c += 32;
	}
    return(c);
}

inline u32
string_kinda_hash(u8 *text)
{
	u8 c = 0;
	u32 i = 0;
	u32 kindaHash = 0;
    while((c = text[i++]) != '\0')
	{
		u32 c32 = (u32)c;
		kindaHash += noise_u32(c32, i);
	}

	return(kindaHash);
}

inline u32
string_kinda_hash_seeded(u8 *text, u32 seed)
{
	u8 c = 0;
	u32 i = 0;
	u32 kindaHash = 0;
    while((c = text[i++]) != '\0')
	{
		u32 c32 = (u32)c;
		kindaHash += noise_u32(c32, seed);
	}

	return(kindaHash);
}

static u32
string_concadenate(
		u8 *s0,
		u8 *s1,
		u8 *textBuffer,
		u32 textBufferLength)
{
	u32 textBufferIndex = 0;
	u32 newLength = 0;

	u8 c  = 0;
	u32 i = 0;
	while((textBufferIndex < textBufferLength) && (c = s0[i]) != '\0')
	{
		textBuffer[textBufferIndex++] = c;
		i++;
	}
	c = 0;
	i = 0;
	while((textBufferIndex < textBufferLength) && (c = s1[i]) != '\0')
	{
		textBuffer[textBufferIndex++] = c;
		i++;
	}
	newLength += textBufferIndex;

	return(newLength);
}

static u32
string_add(u8 *source,
		   u8 *dest,
		   u32 dest_total_size)
{
	u32 i          = 0;
	while(dest[i] != '\0')
	{
		i++;
	}

	u32 s = 0;

	while(i < dest_total_size && 
		  source[s] != '\0')
	{
		dest[i] = source[s];
		i++;
		s++;
	}

	u32 new_length = i;
	return(new_length);
}

inline uint32
string_count(uint8 *s)
{
	uint32 result = 0;
	while(*(s + result++) != '\0');
	return(result);
}

inline u32
string_ends_with_char(u8 *text, u8 with)
{
	u32 result = 0;
	u32 i = 0;
	while(!result && text[i] != '\0')
	{
		result = text[i++] == with;
	}
	return(result);
}

static inline b32
string_ends_with(u8 *text, u8 *with)
{
	u32 with_count = string_count(with);
	u32 text_count = string_count(text);
	u32 i = text_count - 1;
	u32 e = with_count - 1;
	b32 ends_with = text[i] == with[e];
	u8 c = 0;
	while(i && e && ends_with)
	{
		i--;
		e--;
		ends_with &= (text[i] == with[e]);
	}

	return(ends_with);

}

static inline b32
string_starts_with(u8 *text, u8 *with)
{
	u32 with_count = string_count(with);
	u32 text_count = string_count(text);
	if(with_count > text_count)
	{
		return(0);
	}
	if(with_count)
	{
		with_count--;
	}
	u32 i = 0;
	u32 e = 0;
	b32 starts_with = text[i] == with[e];
	u8 c = 0;
	while(i < with_count && e < with_count && starts_with && with[e] != '\0')
	{
		starts_with &= (text[i] == with[e]);
		i++;
		e++;
	}

	return(starts_with);

}
static void
string_remove_and_truncate(u8 *text, u8 charToRemove)
{
	u32 i = 0;
	u32 c = 0;
	while((c = text[i]) != '\0')
	{
		if(c == charToRemove)
		{
			u32 j = i;
			while((text[j] != '\0'))
			{
				text[j] = text[j + 1];
				j++;
			}
		}
		i++;
	}
}
static u32
string_get_next_char_index_from(u8 *text, u32 from, u8 c)
{
    u32 i = from;
	if(text[i] != c && text[i] != '\0')
	{
		i++;
		while(text[i] != c && text[i] != '\0')
		{
			i++;
		}
	}
	return(i);
}

//start from index and decrese until text[i] is the specified character
static u32
string_get_previous_char_index_from(u8 *text, u32 from, u8 c)
{
    u32 i = from;
	if(i && text[i] != c)
	{
		i--;
		while(i && text[i] != c)
		{
			i--;
		}
	}
	return(i);
}

static u32
string_advance_until_and_get_last_char_from_row(u8 *text, u32 from, u8 c)
{
    u32 i = from;
	if(text[i] != c && text[i] != '\0')
	{
		i++;
		while(text[i] != c && text[i] != '\0')
		{
			i++;
		}
	}
	while(text[i] == c && text[i] != '\0')
	{
		i++;
	}
	return(i);
}

static u32
string_go_back_until_and_get_first_char_from_row(u8 *text, u32 from, u8 c)
{
    u32 i = from;
	if(i && text[i] != c)
	{
		i--;
		while(i && text[i] != c)
		{
			i--;
		}
	}
	while(i && text[i] == c)
	{
		i--;
	}
	return(i);
}

static u32
string_count_words(u8 *text)
{
	u32 t = 0;
	u32 w = 0;
	u8 c = text[t];

	while(text[t] != '\0')
	{
		while(text[t] != ' ' && text[t++] != '\0');
		w++;
		t++;
	}

	return(w);
}

inline void
string_clear(u8 *text)
{
	u32 i = 0;
	while(text[i] != '\0')
	{
		text[i] = '\0'; 
		i++;
	}
}

inline u8 * 
string_to_low(u8 *text)
{
	u32 i = 0;
	while(text[i] != '\0')
	{
		text[i] = char_to_low(text[i]);
		i++;
	}
	return(text);
}

inline int32
string_compare(uint8 *f, uint8 *s)
{
    int32 result = 1;
	//These values make sure they both end at the same time.
	u32 atEnd0 = 0;
	u32 atEnd1 = 0;
    while(result && !atEnd0 && !atEnd1)
    {
        atEnd0 = *f == '\0';
		atEnd1 = *s == '\0';
        result &= (*f++ == *s++);
    }
    return(result && (atEnd0 & atEnd1));
}
//Not yet tested
inline int32
string_compareignorecase(uint8 *f, uint8 *s)
{
    int32 result = 1;
    while(result && *f != '\0' && s != '\0')
    {
        result &= (char_to_low(*f++) == char_to_low(*s++));
    }
    return(result);
}

inline uint32
string_copy(uint8 *s, uint8 *d)
{
	uint32 i = 0;
	uint8 c = 0;
	while((c = s[i]) != '\0')
	{
		d[i++] = c;
	}
	d[i] = '\0';
	return(i);
}

static inline u32
string_copy_until(uint8 *s, uint8 *d, u8 until)
{
	uint32 i = 0;
	uint8 c = 0;
	while((c = s[i]) != until && (c != '\0'))
	{
		d[i++] = c;
	}
	d[i] = '\0';
	return(i);
}

static inline u32
string_copy_length(u8 *s, u8 *d, u8 length)
{
	uint32 i = 0;
	uint8 c = 0;
	while((c = s[i]) != '\0' && (i < length))
	{
		d[i++] = c;
	}
	return(i);
}

inline u32
string_copy_word_from_index(u8 *s,
		                   u8 *d,
				           u32 index)
{
	u32 w = 0;
	u32 i = index;
	while(s[i] != ' ' && s[i] != '\0')
	{
		d[w] = s[i];
		i++;
		w++;
	}
	//return the amount of copied characters
	return(w);

}

inline int32
string_copy_and_clear(u8 *src, u8 *dest, u32 destSize)
{
	u32 i = 0;
	while(src[i] != '\0'){
		dest[i] = src[i];
		i++;
	}
	u32 j = i;
	while(j < destSize){
		dest[j] = '\0';
		j++;
	}
	return(i);
}


//
// __string path functions
//

static inline void
path_fill_directory(u8 *file_path_and_name, u8 *dest)
{
	u32 i = 0;
	u32 index_after_end_slash = 0;
	u8 c = 0;
	while((c = file_path_and_name[i]) != '\0')
	{
		if(c == '\\' || c == '/')
		{
			index_after_end_slash = i + 1;
		}
		i++;
	}
	string_copy_length(
			file_path_and_name,
			dest,
			index_after_end_slash);

}

typedef struct{
	u16 name_length;
	u16 extension_length;

	u8 extension[8];
	u8 name[128];
}file_path_name_and_extension_info;

inline file_path_name_and_extension_info
path_get_file_path_info(u8 *file_path_and_name)
{
	u8 c = 0;
	u32 fileNameIndex = 0;
	u32 pathCount = string_count(file_path_and_name);
	u32 i = pathCount - 1;

	u32 extensionIndex = 0;
	u32 fileNameLength = 0;
	u32 fileExtensionLength = 0;

	file_path_name_and_extension_info fileInfo = {0};

	while(!fileNameIndex && i > 0)
	{
        c = file_path_and_name[i];
		//Read until the final slashes to get the file name
		if((c == '\\' || c == '/'))
		{
		    fileNameIndex = i + 1;
		}
		//found extension index
		if(!extensionIndex && c == '.')
		{
			extensionIndex = i;
		}
		i--;
	}
	//Subtract also the '\0'
	fileNameLength = pathCount - fileNameIndex - 1; 

	if(extensionIndex)
	{
		//If an extension was found, subtract it's length from the file name
		
		fileExtensionLength = (pathCount - extensionIndex) - 1;
		fileNameLength     -= fileExtensionLength;
		//Subtract the '.'
		fileExtensionLength -= 1;

	   i = extensionIndex + 1;
	   u32 destIndex = 0;

	   while(i != pathCount)
	   {
	   	  fileInfo.extension[destIndex++] = file_path_and_name[i++];
	   }
	}

	i = fileNameIndex;
	u32 destIndex = 0;
    while(destIndex < fileNameLength)
	{
		fileInfo.name[destIndex++] = file_path_and_name[i++];
	}

	fileInfo.name_length      = fileNameLength;
	fileInfo.extension_length = fileExtensionLength;
	return(fileInfo);
}

static inline void 
path_fill_file_name_and_type(u8 *file_path_and_name, u8 *dest_name, u8 *dest_type)
{
	u8 c = 0;
	u32 fileNameIndex = 0;
	u32 pathCount = string_count(file_path_and_name);
	u32 i = pathCount - 1;

	u32 extensionIndex = 0;
	u32 fileNameLength = 0;
	u32 fileExtensionLength = 0;

	while(!fileNameIndex && i > 0)
	{
        c = file_path_and_name[i];
		//Read until the final slashes to get the file name
		if((c == '\\' || c == '/'))
		{
		    fileNameIndex = i + 1;
		}
		//found extension index
		if(!extensionIndex && c == '.')
		{
			extensionIndex = i;
		}
		i--;
	}
	//Subtract also the '\0'
	fileNameLength = pathCount - fileNameIndex - 1; 

	if(extensionIndex && dest_type)
	{
		//If an extension was found, subtract it's length from the file name
		
		fileExtensionLength = (pathCount - extensionIndex) - 1;
		fileNameLength     -= fileExtensionLength;
		//Subtract the '.'
		fileExtensionLength -= 1;

	   i = extensionIndex + 1;
	   u32 destIndex = 0;

	   while(i != pathCount)
	   {
	   	  dest_type[destIndex++] = file_path_and_name[i++];
	   }
	}

	i = fileNameIndex;
	u32 destIndex = 0;
	if(dest_name)
    while(destIndex < fileNameLength)
	{
		dest_name[destIndex++] = file_path_and_name[i++];
	}
}

inline void
path_fill_file_name(u8 *file_path_and_name, u8 *dest)
{
	file_path_name_and_extension_info fileInfo = path_get_file_path_info(file_path_and_name);
	//fill name
	u32 i = 0;
	u8 c = 0;
	while((c = fileInfo.name[i]) != '\0')
	{
		dest[i] = c;
		i++;
	}
	//fill extension
	if(fileInfo.extension_length)
	{
		dest[i++] = '.';

		u32 extensionIndex = 0;
		while((c = fileInfo.extension[extensionIndex++]) != '\0')
		{
			dest[i] = c;
			i++;
		}
	}

}

static void
path_fill_parent_directory(u8 *path_and_name, u8 *dest)
{
	u32 count = string_count(path_and_name);
	//find last slash
	u32 i = count - 1;

	u8 c = 0;
	while(i)
	{
		c = path_and_name[i];
		if(c == '/' || c == '\\')
		{
			i--;
			//ignore slashes
			while(i && path_and_name[i] == '/' || path_and_name[i] == '\\')
			{
				i--;
			}
			//get to the start of the name
			while(i && path_and_name[i - 1] != '/' && path_and_name[i - 1] != '\\')
			{
				i--;
			}
			//start copying the name
			u32 dest_i = 0;
			while(i < count && path_and_name[i] != '/' &&  path_and_name[i] != '\\')
			{
				dest[dest_i++] = path_and_name[i++];
			}
			break;
		}
		i--;
	}
}

inline void
path_fill_file_name_only(u8 *file_path_and_name, u8 *dest)
{
	file_path_name_and_extension_info fileInfo = path_get_file_path_info(file_path_and_name);
	u32 i = 0;
	u8 c = 0;
	while((c = fileInfo.name[i]) != '\0')
	{
		dest[i] = c;
		i++;
	}
}

inline u32
path_get_extension(u8 *file_path_and_name, u8 *dest, u32 destLength)
{
	u8 c = 0;
	u32 i = 0;
	u32 extensionIndex = 0;
	//find extension index
	while((c = file_path_and_name[i++]) != '\0')
	{
		//What if the file starts with '.'?
		if(c == '.')
		{
			extensionIndex = i - 1;
		}

	}

	u32 extensionCount = 0;
	if(extensionIndex)
	{
		u32 f = 0;
		extensionIndex++;
		while(f < destLength && ((c = file_path_and_name[extensionIndex]) != '\0'))
		{
			dest[f] = c;
			f++;
			extensionIndex++;
		}
		extensionCount = f;
	}
	return(extensionCount);
}

inline u32
path_is_extension(u8 *filePathAndName, u8 *extension)
{
	u8 extensionBuffer[16] = {0};
	path_get_extension(filePathAndName, extensionBuffer, sizeof(extensionBuffer));

	u32 success = string_compare(extensionBuffer, extension);

	return(success);
}

#if 0
inline u32
_path_NameIsValid(u8 *name, u32 length)
{
	u32 valid = 0;
	u32 nameIndex = 0;
	u32 i = 0;

	while(valid && nameIndex < fileInfo.name_length)
	{
		   while(i < forbidenCount)
		   {
			   valid = (fileInfo.name[nameIndex] != forbidenCharacters[i]) && (fileInfo.name[nameIndex] > 31);
			   i++;
		   }
		   nameIndex++;
		   i = 0;
   }
   return(valid);
}
#endif
inline u32 
path_and_name_is_valid(u8 *file_path_and_name)
{
	u32 isValid = 0;
	u32 textCount = string_count(file_path_and_name);
	//Not null
	u8 forbidenCharacters[11] = {"<>:\"/\\|?*"};
	u32 forbidenCount = 11;
	if(textCount > 1)
	{
	   file_path_name_and_extension_info fileInfo = path_get_file_path_info(file_path_and_name);
	   u32 valid     = 1;
	   u32 nameIndex = 0;
	   u32 i         = 0;

	   u8 *name       = fileInfo.name;
	   u32 name_length = fileInfo.name_length;
	   //check valid file name
	   while(valid && nameIndex < fileInfo.name_length)
	   {
		   while(valid && i < forbidenCount)
		   {
			   valid = (name[nameIndex] != forbidenCharacters[i]) && (name[nameIndex] > 31);
			   i++;
		   }
		   nameIndex++;
		   i = 0;
	   }
	   //check valid extension name
	   nameIndex = 0;
	   name       = fileInfo.extension;
	   name_length = fileInfo.extension_length;
	   
       while(valid && nameIndex < name_length)
	   {
		   while(valid && i < forbidenCount)
		   {
			   valid = (name[nameIndex] != forbidenCharacters[i]) && (name[nameIndex] > 31);
			   i++;
		   }
		   nameIndex++;
		   i = 0;
	   }
	   isValid = valid;

	}
	return(isValid);

}

//
// string path functions__
//
inline static void
push_char(char* buffer, uint16 *at, char c)
{
    buffer[*at] = c;
    (*at)++;
}
inline static void 
u64_to_ascii(char *buffer, uint16 *at, uint64 value)
{
   char c = 0;

   uint16 start = *at;
   c = (value % 10) + '0';
   push_char(buffer, at, c);

   while((value = value / 10))
   {
     c = (value % 10) + '0';
     push_char(buffer, at, c);
   }
   uint16 end = *at - 1;
   while(end > start)
   {
      char digit = buffer[start];
      buffer[start++] = buffer[end];
      buffer[end--] = digit;
   }
   
}
inline static void 
u32_to_ascii(char *buffer, uint16 *at, uint32 value)
{
   char c = 0;

   uint16 start = *at;
   c = (value % 10) + '0';
   push_char(buffer, at, c);

   while((value = value / 10))
   {
     c = (value % 10) + '0';
     push_char(buffer, at, c);
   }
   uint16 end = *at - 1;
   while(end > start)
   {
      char digit = buffer[start];
      buffer[start++] = buffer[end];
      buffer[end--] = digit;
   }
   
}
inline static void 
i32_to_ascii(char *buffer, uint16 *at, int value)
{
   char c = 0;

   uint16 start = *at;
   c = (value % 10) + '0';
   push_char(buffer, at, c);

   while((value = value / 10))
   {
     c = (value % 10) + '0';
     push_char(buffer, at, c);
   }

   
   uint16 end = *at - 1;
   while(end > start)
   {
      char digit = buffer[start];
      buffer[start++] = buffer[end];
      buffer[end--] = digit;
   }
   
}
static int 
format_text_list(char* buffer, uint32 buffersize, char* format, va_list args)
{
    char c = format[0];

    uint16 textFormatedAt = 0;

	int32 llFormat = 0;
    for(u32 i = 0;
            i < buffersize;
            i++)
    {
        (c = format[i]);
        //TODO: format character with % char 37
        //NOTE: Get various chars and display them from the same image
        //Step 1: Get the character
       i32 precision = DEFAULT_FORMAT_PRECISION;
       if(c == '%')
       {
           i++;
		   u32 processingFormat = 1;
		   while(processingFormat)
		   {

               switch(format[i])
               {
		           //Precision
		           case '.':
		        	   {
		        		   u32 digitCount = 0;

		        		   u8 digitBuffer[24] = {0};

		        		   c = format[++i];
		        		   while(c >= '0' && c <= '9')
		        		   {

		        			   digitBuffer[digitCount] = c;
		        			   digitCount++;
		        			   i++;
		        			   c = format[i];
		        		   }

		        		   //Precision has sign!
		        		   u32_from_string(digitBuffer, &precision);

		        		   if(precision > 9)
		        		   {
		        			   precision = 9;
		        		   }

		        	   }break;
		           //Specifiers
		          case 'l':
		        	   {
		        			//Dont break, continue
		        		   llFormat = 1;
		        		   while(format[++i] == 'l');
		        		   processingFormat = 0;
		        	   }break;
                   case 's':
                       {

                          char *l = va_arg_m(args, char *); 
                          while(l[0])
                          {
                            push_char(buffer, &textFormatedAt, l[0]);
                            l++;
                          //print
                          }
		        		   processingFormat = 0;
                       }break;
                   case 'c':
                       {

                          char l = va_arg_m(args, int32); 
                          push_char(buffer, &textFormatedAt, l);
		        		   processingFormat = 0;
                       }break;
                  case 'u':
                       {
                           uint32 value = (u32)va_arg_m(args, memory_size);

                           u32_to_ascii(buffer ,&textFormatedAt, value);
		        		   processingFormat = 0;
                       }break;
                  case 'd':
                       {
                           int32 value = (i32)va_arg_m(args, int64);

                           if(value < 0)
                           {
                              push_char(buffer, &textFormatedAt, '-');
                              value = -value;
                           }
                           i32_to_ascii(buffer , &textFormatedAt, value);
		        		   processingFormat = 0;

                       }break;
                  case 'f':
                       {
                           float value = (float)va_arg_m(args, double);
                           if(value < 0)
                           {
                               push_char(buffer, &textFormatedAt, '-');
                               value = -value;
                           }
                           i32 valueInteger = (int)value;
		        		   //Get rid of decimals
                           value           -= valueInteger;
		        		   //Push the whole number
                           i32_to_ascii(buffer , &textFormatedAt, valueInteger);


                           push_char(buffer, &textFormatedAt, '.');
                           for(int p = 0;
                                   p < precision;
                                   p++)
                           {
                              value *= 10;
		        			  //Add the '0' char to place the value on the correct place
                              uint8 value8 = ((uint32)value % 10) + '0'; 
                              push_char(buffer, &textFormatedAt, value8);
                           }
		        		   processingFormat = 0;

                       }break;
		          case 'n':
		        	   {
		        		   //Not implemented yet
		        		   processingFormat = 0;

		        	   }break;
				  default:
					   {
						   
						   //Assert(0);
						   processingFormat = 0;
					   }
                   
               }
		   }
       }
       else if(c == '\0')
       {
           push_char(buffer, &textFormatedAt, c);
           break;
       }
       else 
       {
           push_char(buffer, &textFormatedAt, c);
       }

    }
    uint8 *lastc = buffer;
    uint32 TextSize = 1;
    while(*lastc != '\0')
    {
        lastc++;
        TextSize++;
    }
    return TextSize;
}

#define FORMAT_TEXT_QUICK(quick_format, text)\
	va_list args;\
	va_start_m(args, text);\
	quick_format.text_length = format_text_list(quick_format.text_buffer, sizeof(quick_format.text_buffer), text, args);\
	va_end_m(args);

static int
format_text(char* buffer, uint32 buffersize, char* format, ...)
{
    va_list args;
    va_start_m(args, format);
    u32 TextSize = format_text_list(buffer, buffersize, format, args);
    va_end_m(args);
    return TextSize;
}
