static inline void
RadixSortDraws(uint8 *sortstorage, uint8 *tempstorage, int count)
{
   uint8 *source      = sortstorage;
   uint8 *destination = tempstorage;

   for(int byte = 0; 
           byte < 40;
           byte += 8)
   {

       uint32 RadixOffsets[256] = {0};

       for(int i = 0; 
               i < count;
               i++)
       {
           uint32 value = 1;
           int rindex = (value >> byte) & 0xff; //Sorts in ascending order
           RadixOffsets[rindex]++;
       }
       //Second phase, sort and count the radix indexes.
       int total = 0;
       for(int RadixI = 0;
               RadixI < 256;
               RadixI++)
       {
           int c = RadixOffsets[RadixI];
           RadixOffsets[RadixI] = total;
           total += c;
       }
       //Last phase: sort the destination and source
       for(int i = 0;
               i < count; 
               i++)
       {
           uint32 value = 1;
           int rindex =  (value >> byte) & 0xff;
           destination[RadixOffsets[rindex]++] = source[i];
       }
       //Swap source and destination
       uint8 *tempcopy = destination;
       destination = source;
       source = tempcopy;
   }

}
static inline void
InsertionSortDraws(uint8 *renderstorage, uint32 count)
{
    for(uint32 a = 0; a < count; a++)
    {
        for(uint32 b = a + 1; b < count; b++)
        {
#if 0
            if(renderstorage->sorts[a].depth > renderstorage->sorts[b].depth)
            {
               RenderSort dataRemainder  = renderstorage->sorts[b];
               renderstorage->sorts[b]      = renderstorage->sorts[a];
               renderstorage->sorts[a]      = dataRemainder;

            }
#endif
        }
    }

}
//TODO: Finish
//Pointer to the place where the FIRST element of the split array is
#if 0
static inline void
Merge(RenderStorage *sortstorage, int startindex, int halfindex, int endindex)
{
    int startindex2 = halfindex + 1;
    if(sortstorage->sorts[halfindex].depth < sortstorage->sorts[startindex2].depth)
    {
    }
    else
    {
        while(startindex <= halfindex && startindex2 <= endindex)
        {

            if(sortstorage->sorts[startindex].depth < sortstorage->sorts[startindex2].depth)
            {
                startindex++;
            }
            else
            {
                RenderSort data = sortstorage->sorts[startindex2];
                int i = startindex2;

                while(i != startindex) 
                {
                    sortstorage->sorts[i] = sortstorage->sorts[i - 1];
                    i--;
                }
                sortstorage->sorts[i] = data;
                startindex++;
                halfindex++;
                startindex2++;


            }
        }
    }

}
static inline void 
MergeSortData(RenderStorage *sortstorage, int startindex, int endindex)
{

    if(startindex < endindex){
        int halfindex = startindex + (endindex - startindex) / 2;
        MergeSortData(sortstorage, startindex, halfindex);
        MergeSortData(sortstorage, halfindex + 1, endindex);
        Merge(sortstorage, startindex, halfindex, endindex);

    }
    
}
#endif

static inline uint32
RealToInt32(real32 value)
{
    uint32 result = *(uint32 *)&value;
   //for checking negative values
    if(result & 0x80000000)
    {
        result = ~result;
    }
    return result;
}

static inline void
RadixSortVertices(SpriteQuad *sortstorage, SpriteQuad *tempstorage, int count)
{
   SpriteQuad *source      = sortstorage;
   SpriteQuad *destination = tempstorage;

   for(int byte = 0; 
           byte < 40;
           byte += 8)
   {

       uint32 RadixOffsets[256] = {0};

       for(int i = 0; 
               i < count;
               i++)
       {
           uint32 value = RealToInt32(source[i].vertices[0].location.z);
           int rindex = (value >> byte) & 0xff; //Sorts in ascending order
           rindex = 0xff - rindex; //switch the order
           RadixOffsets[rindex]++;
       }
       //Second phase, sort and count the radix indexes.
       int total = 0;
       for(int RadixI = 0;
               RadixI < 256;
               RadixI++)
       {
           int c = RadixOffsets[RadixI];
           RadixOffsets[RadixI] = total;
           total += c;
       }
       //Last phase: sort the destination and source
       for(int i = 0;
               i < count; 
               i++)
       {
           uint32 value = RealToInt32(source[i].vertices[0].location.z);
           int rindex =  (value >> byte) & 0xff;
           destination[RadixOffsets[rindex]++] = source[i];
       }
       //Swap source and destination
       SpriteQuad *tempcopy = destination;
       destination = source;
       source      = tempcopy;
   }

}
