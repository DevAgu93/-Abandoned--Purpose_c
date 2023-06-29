//Windows-specific IO
//#include <windows.h>
//Pixel data: (IsCube, IsFacingCamera, TopFaceU, TopFaceV, TopFaceU, TopFaceV, TopFaceU, TopFaceV, TopFaceU, TopFaceV)

typedef struct {
   uint64 size;
   char* contents;
}FileInfo;

FileInfo
PlatformReadEntireFile(char* name);

static
uint32 ReadIntFromElement(char* content, int index)
{
    int32 number = 0;
    int start = index;
    char c = content[index];
    while(((c = content[index]) >= '0' && c <= '9'))
    {
       number = (number * 10) + (c - '0');
       index++;
    }
    return number;
}
inline static
void ValuesToArr32(char* filecontent, int*array, int index, int ArrayLength)
{
   char c = 0;
   int arrindex = 0;
   while((c = filecontent[index]) != '\0' && c != ']' && arrindex < ArrayLength)
   {
       if(c >= '0' && c <= '9')
       {
           array[arrindex] = 0;
          while(c >= '0' && c <= '9')
          {
             array[arrindex] = array[arrindex] * 10 + (c - '0');
             index++;
             c = filecontent[index];
          }
          arrindex++;
       }
       else
       {
         index++;
       }
   }
}
inline static
void ValuesToArr16(char* filecontent, int16 *array, int index, int ArrayLength)
{
   char c = 0;
   int arrindex = 0;
   while((c = filecontent[index]) != '\0' && c != ']' && arrindex < ArrayLength)
   {
       if(c >= '0' && c <= '9')
       {
          array[arrindex] = 0;
          while(c >= '0' && c <= '9')
          {
             array[arrindex] = array[arrindex] * 10 + (c - '0');
             index++;
             c = filecontent[index];
          }
          arrindex++;
       }
       else
       {
         index++;
       }
   }
}
inline static
int32 ReadArray32(char *file, int32 *array, int element, int ArrayLenght)
{
    char c = 0;
    int index = 0;
    int SelectedElement = 0;
    while((c = file[index]) != '\0' && c != ']' && index < ArrayLenght)
    {
        if(c == ':')
        {
           SelectedElement++;
        }
        if(SelectedElement == element)
        {
           index++;
           while((c = file[index]) == ' ')
           {
               index++;
           }
           ValuesToArr32(file, array, index, ArrayLenght);
           break;
        }
        index++;
    }
    return 1;
}
inline static
int32 ReadArray16(char *file, int16 *array, int element, int ArrayLenght)
{
    char c = 0;
    int index = 0;
    int SelectedElement = 0;
    while((c = file[index]) != '\0' && c != ']' && index < ArrayLenght)
    {
        if(c == ':')
        {
           SelectedElement++;
        }
        if(SelectedElement == element)
        {
           index++;
           while((c = file[index]) == ' ')
           {
               index++;
           }
           ValuesToArr16(file, array, index, ArrayLenght);
           break;
        }
        index++;
    }
    return 1;
}
//NOTE: I'll probably let this here for a while
static
int ReadIntElement(char *file, int element)
{
    char c = '0';
    int value = 0;
    int index = 0;
    int SelectedElement = 0;
    while((c = file[index]) != '\0')
    {
        if(c == ':')
        {
           SelectedElement++;
        }
        if(SelectedElement == element)
        {
           index++;
           while((c = file[index]) == ' ')
           {
               index++;
           }
            value = ReadIntFromElement(file, index);
            break;
        }
        index++;
    }
    return value;
}
/*
static
int32 fGetFileSize(void *file)
{
   
    SDL_RWseek(file, 0 ,RW_SEEK_END);
    int32 size = (int32)SDL_RWtell(file);
    SDL_RWseek(file, 0, RW_SEEK_SET);
    return size;
}
*/




#if 0
//Assets loading
void LoadScene(GameWorld *world, char* path)
{
   //TODO: Initialize multiple layers!
   Scene *scene      = world->scene;
   uint32 TotalSize  = sizeof(Scene);
   GfxLayer *layer   = &scene->Graphics[0];
   FileInfo fileinfo = PlatformReadEntireFile(path);

   
   layer->width  = ReadIntElement(fileinfo.contents, 1);
   layer->height = ReadIntElement(fileinfo.contents, 2);
   int DataSize  = layer->width * layer->height;
   //layer->data   = SDL_calloc(DataSize, sizeof(int));
   layer->data   = (uint16 *)(scene + 1);
   TotalSize     += sizeof(int16) * layer->width * layer->height * 2;

   ReadArray16(fileinfo.contents, layer->data, 3, DataSize);
   //TODO:Load tilesize for different layers
   layer->tilesize = 16;
   Assert(TotalSize < scene->Size);
}
#endif
