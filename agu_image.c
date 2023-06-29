/*
   Definitions
Symmetric: A shape is symmetric when, divided by a line, the other side is identical.
   */

typedef struct{
    uint32 width;
    uint32 height;
    uint32 bpp;
    uint8 *pixels;
}image_data;

#define sdf_infinite (1 << 30)

inline vec4
rgba_unpack(uint32 pack)
{
    vec4 result = {
        (real32)((pack >> 0) & 0xff),
        (real32)((pack >> 8) & 0xff),
        (real32)((pack >> 16) & 0xff),
        (real32)((pack >> 24) & 0xff)
    };
    return(result);
}
inline uint32
RGBAPack(vec4 color)
{
    uint32 result = (((uint32)color.x << 0) |
                     ((uint32)color.y << 8) |
                     ((uint32)color.z << 16) |
                     ((uint32)color.w << 24));
    return(result);
}
inline vec4
RGBA255To1(vec4 v)
{
    real32 oneOverMaxColor = 1.0f / 255.0f;
    v.x *= oneOverMaxColor;
    v.y *= oneOverMaxColor;
    v.z *= oneOverMaxColor;
    v.w *= oneOverMaxColor;
    return(v);
}
inline vec4
RGBA1To255(vec4 v)
{
    real32 MaxColor = 255; 
    v.x *= MaxColor;
    v.y *= MaxColor;
    v.z *= MaxColor;
    v.w *= MaxColor;
    return(v);
}

static image_data 
image_OffsetSpriteSheet(image_data *image, memory_area *tempArea, u32 frameW, u32 frameH, u32 pixelOffsetX, u32 pixelOffsetY, u32 fromCorner)
{
    //384x348
    uint32 width  = image->width;
    uint32 height = image->height;

	u32 newFrameW = frameW + pixelOffsetX;
	u32 newFrameH = frameH + pixelOffsetY;
    u32 totalFramesX = image->width  / newFrameW;
    u32 totalFramesY = image->height / newFrameH;
    //totalFramesX = 1;
    //totalFramesY = 1;
	image_data destinationImage = {0};
	destinationImage.width      = image->width;
	destinationImage.height     = image->height;
	destinationImage.bpp        = image->bpp;
	destinationImage.pixels     = memory_area_push_size(tempArea, image->width * image->height * image->bpp);
	u8 *destination             = destinationImage.pixels; 

    uint32 *at   = 0;
    uint32 *to   = 0;

	u32 imageStride = image->width * image->bpp;
    u32 *start = (uint32 *)image->pixels;
	u32 *startRowSrc = 0;
	u32 *startRowDst = 0;

	u32 halfOffsetX = pixelOffsetX / 2;
	u32 halfOffsetY = pixelOffsetY / 2;

	u32 *dstStart = (u32 *)destinationImage.pixels;
	if(!fromCorner)
	{
		dstStart += (halfOffsetX + width * halfOffsetY);
	}
    for(uint32 fy = 0; fy < totalFramesY; fy++)
    {
		//Both share the same size
        startRowSrc = (u32 *)image->pixels + (width * (frameH * fy));
        startRowDst = dstStart             + (width * (newFrameH * fy));

        to = startRowDst; 
        at = startRowSrc; 

        for(uint32 fx = 0; fx < totalFramesX; fx++)
        {

            u32 *atStart = at;
            u32 *toStart = to;
			//Copy the whole tile
			//to += halfOffsetX + width * halfOffsetY;

            for(uint32 newFrameIndexY = 0; 
                       newFrameIndexY < frameH;
                       newFrameIndexY++)
            {
                for(uint32 newFrameIndexX = 0; 
                           newFrameIndexX < frameW;
                           newFrameIndexX++)
                {
                    to[newFrameIndexX] = at[newFrameIndexX];
                }
                at = at + width;
                to = to + width;
            }
            at = atStart + frameW;
            to = toStart + newFrameW;

        }
    }

	return(destinationImage);
}

static image_data 
image_set_tiles_for_blending(image_data *image, memory_area *tempArea, u32 frameW, u32 frameH, u32 fromCorner)
{
	u32 pixelOffsetX = 2;
	u32 pixelOffsetY = 2;

    image_data result = image_OffsetSpriteSheet(image, tempArea, frameW, frameH, pixelOffsetX, pixelOffsetY, fromCorner);
    //384x348
    uint32 width  = image->width;
    uint32 height = image->height;

	u32 newFrameW = frameW + pixelOffsetX;
	u32 newFrameH = frameH + pixelOffsetY;
    u32 totalFramesX = image->width  / newFrameW;
    u32 totalFramesY = image->height / newFrameH;

    uint32 *at   = 0;
    uint32 *to   = (u32 *)result.pixels;

	u32 *startRow = 0;

	u32 toOffsetX = 0;
	u32 toOffsetY = 0;
		toOffsetX = pixelOffsetX;
		toOffsetY = pixelOffsetY;

	u32 *atStart = (u32 *)result.pixels;
	u32 skipFirstRow = fromCorner;
	if(!skipFirstRow)
	{
	  atStart += width;
	}

    for(uint32 y = 0; y < totalFramesY; y++)
    {
		//Both share the same size
        u32 *startAt  = atStart + (width * (newFrameH * y));

        at = startAt;
		if(skipFirstRow)
		{
		  skipFirstRow = 0;
		}
		else
		{
		  to = at - width;
		}

		for(u32 ox = 0; ox < 2; ox++)
		{
          for(uint32 fx = 0; fx < width; fx++)
          {
		  	  to[fx] = at[fx];
          }
		  at += width * (frameW - 1);
		  to = at + width;
		}
    }

	atStart  = (u32 *)result.pixels;

	u32 skipFirstColumn = fromCorner;
	if(!skipFirstColumn)
	{
		atStart += 1;
	}
    for(u32 x = 0; x < totalFramesX; x++)
	{
       at = atStart + (newFrameW * x);
	   if(skipFirstColumn)
	   {
		   skipFirstColumn = 0;
	   }
	   else
	   {
         to = at - 1;
	   }
	   for(u32 oy = 0; oy < 2; oy++)
	   {
         for(uint32 fy = 0; fy < height; fy++)
         {
	     	  to[fy * width] = at[fy * width];
         }
	     at += frameW - 1;
	     to = at + 1; 
	   }
	}
	return(result);
}

//Note(Agu): bounds is an "offset" and "size limit" inside the image.
static void
OffsetSpriteSheet(image_data *image,
                  uint32 oldFrameW, uint32 oldFrameH,
                  uint32 newFrameX, uint32 newFrameY, uint32 newFrameW, uint32 newFrameH,
                  uint32 boundsX, uint32 boundsY, uint32 boundsW, uint32 boundsH)
                  
{
    //384x348
    uint32 width  = image->width;
    uint32 height = image->height;

    uint32 totalFramesX = boundsW / oldFrameW;
    uint32 totalFramesY = boundsH / oldFrameH;
    //totalFramesX = 1;
    //totalFramesY = 1;

    uint32 *at   = 0;
    uint32 *from = 0;
    uint32 *to   = 0;
    uint32 *start = (uint32 *)image->pixels;

    for(uint32 fy = 0; fy < totalFramesY; fy++)
    {
        from = start + (width * (fy * oldFrameH));
        to   = start + (width * (fy * newFrameH));
        at = from + newFrameX + (newFrameY * width); 
        for(uint32 fx = 0; fx < totalFramesX; fx++)
        {
            from = from + oldFrameW;
            uint32 *toStart = to;
            for(uint32 newFrameIndexY = 0; 
                       newFrameIndexY < newFrameH;
                       newFrameIndexY++)
            {

                at = at + width;
                to = to + width;
                for(uint32 newFrameIndexX = 0; 
                           newFrameIndexX < newFrameW;
                           newFrameIndexX++)
                {
                    to[newFrameIndexX] = at[newFrameIndexX];
                }
            }
            at = from + newFrameX + (newFrameY * width); 
            to = toStart + newFrameW;

        }
    }

}

static void
apply_box_filter_to_pixels(
		u32 image_width,
		u32 image_height,
		u8 *pixels)
{
     u32 *pixels_32 = (u32 *)pixels; 
     u32 *to     = pixels_32; 

     for(u32 y = 0;
			 y < image_height;
			 y++)
     {
        for(u32 x = 0; x < image_width; x++)
        {
			u32 r = 0;
			u32 g = 0;
			u32 b = 0;
			u32 a = 0;
			u32 box_counter = 0;

			i32 pixel_y = (i32)y - 1;
			while(pixel_y < (i32)y + 2)
			{
				if(pixel_y < 0)
				{
					pixel_y = 0;
				}
				if(pixel_y == image_height)
				{
					break;
				}
				i32 pixel_x = (i32)x - 1;
				while(pixel_x < (i32)x + 2)
				{
					if(pixel_x < 0)
					{
						pixel_x = 0;
					}
					else if(pixel_x == image_width)
					{
						break;
					}

					u32 pixel_at_color = *(pixels_32 + pixel_x + (pixel_y * image_width));
					vec4 pixel_color   = rgba_unpack(pixel_at_color);

					r += (u32)pixel_color.x;
					g += (u32)pixel_color.y;
					b += (u32)pixel_color.z;
					a += (u32)pixel_color.w;

					box_counter++;
					pixel_x++;
				}
				pixel_y++;
			}

            vec4 final_pixel_color_v4 = {0};
            final_pixel_color_v4.x = (f32)r / box_counter;
            final_pixel_color_v4.y = (f32)g / box_counter;
            final_pixel_color_v4.z = (f32)b / box_counter;
            final_pixel_color_v4.w = (f32)a / box_counter;

            u32 final_pixel_color = RGBAPack(final_pixel_color_v4); 
    
           *to++ = final_pixel_color;
        }
     }
}

static void
apply_linear_filter_to_pixels(
		u32 image_width,
		u32 image_height,
		u8 *pixels)
{
     u32 *pixels_32 = (u32 *)pixels; 
     u32 *to     = pixels_32; 

     for(u32 y = 0;
			 y < image_height;
			 y++)
     {
        for(u32 x = 0; x < image_width; x++)
        {
			u32 r = 0;
			u32 g = 0;
			u32 b = 0;
			u32 a = 0;
			u32 box_counter = 0;

			i32 pixel_y = (i32)y - 0;
			while(pixel_y < (i32)y + 2)
			{
				if(pixel_y < 0)
				{
					pixel_y = 0;
				}
				if(pixel_y == image_height)
				{
					break;
				}
				i32 pixel_x = (i32)x - 0;
				while(pixel_x < (i32)x + 2)
				{
					if(pixel_x < 0)
					{
						pixel_x = 0;
					}
					else if(pixel_x == image_width)
					{
						break;
					}

					u32 pixel_at_color = *(pixels_32 + pixel_x + (pixel_y * image_width));
					vec4 pixel_color   = rgba_unpack(pixel_at_color);

					r += (u32)pixel_color.x;
					g += (u32)pixel_color.y;
					b += (u32)pixel_color.z;
					a += (u32)pixel_color.w;

					box_counter++;
					pixel_x++;
				}
				pixel_y++;
			}

            vec4 final_pixel_color_v4 = {0};
            final_pixel_color_v4.x = (f32)r / box_counter;
            final_pixel_color_v4.y = (f32)g / box_counter;
            final_pixel_color_v4.z = (f32)b / box_counter;
            final_pixel_color_v4.w = (f32)a / box_counter;

            u32 final_pixel_color = RGBAPack(final_pixel_color_v4); 
    
           *to++ = final_pixel_color;
        }
     }
}

static void 
down_sample_image_linear_x2(image_data *image)
                       
{

  uint32 DownScaledWidth    = image->width / 2;
  uint32 DownScaledHeight   = image->height / 2;
  uint32 stride = image->width * image->bpp;

      uint32 *to = (uint32 *)image->pixels;
      uint32 *at = (uint32 *)image->pixels;


      uint32 *at_row1 = at;
      uint32 *at_row_2 = at + image->width;
     for(uint32 y = 0; y < DownScaledHeight; y++)
     {
      // at = image->pixels + (stride * y);
        at_row1 = at;
        at_row_2 = at + image->width;
        for(uint32 x = 0; x < DownScaledWidth; x++)
        {
#if 0
            vec4 tl = rgba_unpack(*at_row1++); 
            vec4 tr = rgba_unpack(*at_row1++);
            vec4 bl = rgba_unpack(*at_row_2++); 
            vec4 br = rgba_unpack(*at_row_2++);
            tl = RGBA255To1(tl);
            tr = RGBA255To1(tr);
            bl = RGBA255To1(bl);
            br = RGBA255To1(br);

            vec4 final_pixel_color_v4 = {0};
            final_pixel_color_v4.x = 0.25f * (tl.x + tr.x + bl.x + br.x);
            final_pixel_color_v4.y = 0.25f * (tl.y + tr.y + bl.y + br.y);
            final_pixel_color_v4.z = 0.25f * (tl.z + tr.z + bl.z + br.z);
            final_pixel_color_v4.w = 0.25f * (tl.w + tr.w + bl.w + br.w);
            final_pixel_color_v4 = RGBA1To255(final_pixel_color_v4);
            uint32 pixel = RGBAPack(final_pixel_color_v4); 
#endif
            vec4 tl = rgba_unpack(*at_row1++); 
            vec4 tr = rgba_unpack(*at_row1++);
            vec4 bl = rgba_unpack(*at_row_2++); 
            vec4 br = rgba_unpack(*at_row_2++);
            vec4 final_pixel_color_v4 = {0};
            final_pixel_color_v4.x = 0.25f * (tl.x + tr.x + bl.x + br.x);
            final_pixel_color_v4.y = 0.25f * (tl.y + tr.y + bl.y + br.y);
            final_pixel_color_v4.z = 0.25f * (tl.z + tr.z + bl.z + br.z);
            final_pixel_color_v4.w = 0.25f * (tl.w + tr.w + bl.w + br.w);

            uint32 pixel = RGBAPack(final_pixel_color_v4); 
    
           *to++ = pixel;
        }
		//advance two rows
        at += (image->width * 2);
     }
	 //set new scale
     image->width  = DownScaledWidth;
     image->height = DownScaledHeight;

}

inline void
down_sample_image_linear(image_data *image, uint32 DownScaleCount)
{
  while(DownScaleCount--)
  {
    down_sample_image_linear_x2(image);
  }
}

inline void
down_sample_pixels(
		u32 w,
		u32 h,
		u8 *pixels,
		u32 amount)
{
	image_data image = {0};
	image.width = w;
	image.height = h;
	image.pixels = pixels;
	image.bpp = 4;
    down_sample_image_linear(&image, amount);
}

static void 
down_sample_image_point(image_data *image,
                     uint32 DownScaleAmount)
{

  uint32 DownScaledWidth    = image->width;
  uint32 DownScaledHeight   = image->height;
  uint32 stride = image->width * image->bpp;


  while(--DownScaleAmount)
  {
      uint8 *to = image->pixels;
      uint8 *at = image->pixels;
      stride = DownScaledWidth * image->bpp;
      DownScaledWidth /= 2;
      DownScaledHeight /= 2;
     for(uint32 y = 0; y < DownScaledWidth; y++)
     {
      // at = image->pixels + (stride * y);
      for(uint32 x = 0; x < DownScaledHeight; x++)
       {
          uint32 pixel = *((uint32 *)at); 
    
         *((uint32 *)to) = pixel;
          at += 8;
          to += 4;
      }
      at += stride;
     }
     image->width  = DownScaledWidth;
     image->height = DownScaledHeight;
  }

}
static image_data 
up_scale_image_point(memory_area *area,
                   image_data *image)
                   
{

    uint32 scale = 2;
  Assert(image->bpp == 4);
  image_data result = {0};

  uint32 ScaledWidth    = image->width  * scale;
  uint32 ScaledHeight   = image->height * scale;
  uint32 ImageSize      = ScaledHeight  * (ScaledWidth * image->bpp);

  uint8 *ImageContents = memory_area_push_size(area, ImageSize);

  uint32 stride = ScaledWidth * image->bpp;
  uint8 *to = ImageContents;
  uint8 *at = image->pixels;

  for(uint32 y = 0; y < image->height; y++)
  {
     to = ImageContents + (stride * y * scale);
     for(uint32 x = 0; x < image->width; x++)
     {
         uint32 pixel = *((uint32 *)at); 

        *((uint32 *)to)               = pixel;
        *((uint32 *)to+1)             = pixel;
        *((uint32 *)to+ScaledWidth)   = pixel;
        *((uint32 *)to+1+ScaledWidth) = pixel;
        at += 4;
        to += 8;
     }
  }
  result.width  = ScaledWidth;
  result.height = ScaledHeight;
  result.bpp    = image->bpp;
  result.pixels = ImageContents;
  return(result);

}

//
// image/pixels functions
//

inline void
draw_line_to_pixels(
		i32 p0_x, i32 p0_y,
		i32 p1_x, i32 p1_y,
		u32 image_width,
		u32 image_height,
		u32 pixel_color,
		u8 *pixels)
{
	vec2 p0 = {(f32)p0_x, (f32)p0_y};
	vec2 p1 = {(f32)p1_x, (f32)p1_y};

	vec2 distance_p1_p0 = {
	(f32)(p1_x - p0_x),
	(f32)(p1_y - p0_y)};
	vec2 p1_p0_normal = vec2_normalize_zero(distance_p1_p0);

	i32 x = p0_x;
	i32 y = p0_y;
	i32 delta_x = p0_x < p1_x ? 1 : -1;
	i32 delta_y = p0_y < p1_y ? 1 : -1;

	u32 *pixels_32 = (u32 *)pixels;
	vec2 ref = p0;
	do
	{
		//p0.x = (f32)x;
		//p0.y = (f32)y;
		ref.x += p1_p0_normal.x;
		ref.y += p1_p0_normal.y;

		vec2 next_p0 = {(f32)x + delta_x, (f32)y};
		vec2 next_p1 = {(f32)x, (f32)y + delta_y};
		vec2 next_p2 = {next_p0.x, next_p1.y}; 

		f32 next_inner0 = vec2_inner(vec2_normalize_zero(vec2_sub(next_p0, p0)), p1_p0_normal);
		f32 next_inner1 = vec2_inner(vec2_normalize_zero(vec2_sub(next_p1, p0)), p1_p0_normal);
		f32 next_inner2 = vec2_inner(vec2_normalize_zero(vec2_sub(next_p2, p0)), p1_p0_normal);

		next_inner0 *= next_inner0;
		next_inner1 *= next_inner1;
		next_inner2 *= next_inner2;

		//put pixel
		*(pixels_32 + x + (y * image_width)) = pixel_color;
		if(x == p1_x && y == p1_y)
		{
			break;
		}
		if(next_inner2 > next_inner0 && next_inner2 > next_inner1)
		{
			
		    	x += delta_x;
		    	y += delta_y;
		}
		else
		{
		    if(next_inner0 > next_inner1)
		    {
		    	x += delta_x;
		    }
		    else
		    {
		    	y += delta_y;
		    }
		}
	}while(1);
}

inline void
draw_line_to_pixels_ddu(
		i32 p0_x, i32 p0_y,
		i32 p1_x, i32 p1_y,
		u32 image_width,
		u32 image_height,
		u32 pixel_color,
		u8 *pixels)
{
  
  f32 step = 0;
  f32 dx = (f32)(p1_x - p0_x);
  f32 dy = (f32)(p1_y - p0_y);

  //determine the distance by the individual coordinates
  if (ABS(dx) >= ABS(dy))
  {
    step = ABS(dx);
  }
  else
  {
    step = ABS(dy);
  }

  dx = dx / step;
  dy = dy / step;
  f32 x = (f32)p0_x;
  f32 y = (f32)p0_y;
  u32 i = 1;
  u32 *pixels_32 = (u32 *)pixels;
  while (i <= step)
  {
	  i32 x_i = (i32)x;
	  i32 y_i = (i32)y;
	  *(pixels_32 + x_i + (y_i * image_width)) = pixel_color;
      x += dx;
      y += dy;
      i++;
  }
}


#define draw_line_to_pixels_bresenham_f32(p0_x, p0_y, p1_x, p1_y, image_width, image_height, pixel_color, pixels) \
    draw_line_to_pixels_bresenham((i32)p0_x, (i32)p0_y, (i32)p1_x, (i32)p1_y, image_width, image_height, pixel_color, pixels)

inline void
draw_line_to_pixels_bresenham(
		i32 p0_x, i32 p0_y,
		i32 p1_x, i32 p1_y,
		u32 image_width,
		u32 image_height,
		u32 pixel_color,
		u8 *pixels)
{
    f32 dx = (f32)ABS(p0_x - p1_x);
    f32 dy = (f32)-ABS(p0_y - p1_y);

	//displacements
	f32 delta_x = p0_x < p1_x ? 1.0f : -1.0f;
	f32 delta_y = p0_y < p1_y ? 1.0f : -1.0f;

	//use as the "total" distance
    f32 error = dx + dy;
    
	f32 x = (f32)p0_x;
	f32 y = (f32)p0_y;

    u32 *pixels_32 = (u32 *)pixels;
	while(1)
	{
	    i32 x_i = (i32)x;
	    i32 y_i = (i32)y;
	    *(pixels_32 + x_i + (y_i * image_width)) = pixel_color;
		if(x == p1_x && y == p1_y)
		{
			break;
		}

        f32 e2 = 2 * error;

        if(e2 >= dy)
		{
            if(x == p1_x)
			{
				break;
			}
            error += dy;
            x     += delta_x;
		}
        if(e2 <= dx)
		{
            if(y == p1_y)
			{
				break;
			}
            error += dx;
			y     += delta_y;
		}

	}
}

inline void
draw_curve_to_pixels_bresenham(
		vec2 p0,
		vec2 p_interpolation,
		vec2 p1,
        u32 curve_color,
		u32 precision,
		u32 image_width,
		u32 image_height,
		u8 *pixels)
{
	//start from the end
	//i32 x = (i32)(p0.x - p_interpolation.x);
	//i32 y = (i32)(p0.y - p_interpolation.y);

	//f64 t = p0.x - 2 * p_interpolation.x + p1.x;
	//f64 r = 0;
	//if(i64)
}

inline void
draw_curve_to_pixels_subdivided(
		vec2 p0,
		vec2 p_interpolation,
		vec2 p1,
        u32 curve_color,
		u32 precision,
		u32 image_width,
		u32 image_height,
		u8 *pixels)
{
	f32 t_advance = 0.5f / precision;
	f32 t = 0;
	//for debugging
	f32 t_before = 0;

	while(t < 1.0f)
	{
		t_before = t;
	    vec2 draw_p0 = vec2_curve_lerp(p0, p_interpolation, p1, t);
		t += t_advance;
	    vec2 draw_p1 = vec2_curve_lerp(p0, p_interpolation, p1, t);


        draw_line_to_pixels_bresenham_f32(
        		draw_p0.x,
				draw_p0.y,
        		draw_p1.x,
				draw_p1.y,
        		image_width,
        		image_height,
        		curve_color,
        		pixels);
	}
}

inline void
draw_curve_to_pixels_interpolated(
		vec2 p0,
		vec2 p_interpolation,
		vec2 p1,
        u32 curve_color,
		u32 image_width,
		u32 image_height,
		u8 *pixels)
{
	f32 t_advance = 0.0005f;
	f32 t = 0;

    u32 *pixels_32 = (u32 *)pixels;
	while(t <= 1.0f)
	{
	    vec2 draw_p0 = vec2_curve_lerp(p0, p_interpolation, p1, t);
		t += t_advance;

		i32 p_x = (i32)(draw_p0.x);
		i32 p_y = (i32)(draw_p0.y);

	    *(pixels_32 + p_x + (p_y * image_width)) = curve_color;
	}
}

inline void
draw_curve_to_pixels_interpolated_AA(
		vec2 p0,
		vec2 p_interpolation,
		vec2 p1,
        u32 curve_color,
		u32 image_width,
		u32 image_height,
		u8 *pixels)
{
	f32 t_advance = 0.0005f;
	f32 t = 0;

    u32 *pixels_32 = (u32 *)pixels;
	while(t <= 1.0f)
	{
	    vec2 draw_p0 = vec2_curve_lerp(p0, p_interpolation, p1, t);
		t += t_advance;

		i32 p_x = (i32)(draw_p0.x);
		i32 p_y = (i32)(draw_p0.y);

		f32 distance_x = ABS((draw_p0.x - (p_x + 1.0f)));
		f32 distance_y = ABS((draw_p0.y - (p_y + 1.0f)));

		f32 percentage_x = 1.0f - distance_x;
		f32 percentage_y = 1.0f - distance_y;

		u32 p_c = (u32)(curve_color * (percentage_x * percentage_y));
		if(distance_x < 0.25f && distance_y < 0.25f)
		{
	        *(pixels_32 + p_x + (p_y * image_width)) = curve_color;
		}
		else
		{
		    u32 *pixel_location = (pixels_32 + p_x + (p_y * image_width));
			if(p_c > (*pixel_location))
			{
	            *pixel_location = p_c;
			}
		}
	}
}

inline u8 *
get_pixel_coordinates_from(
		u32 x,
		u32 y,
		u32 w,
		u32 h,
		u8 *src_pixels)
{
	u32 *src_32 = (u32 *)(src_pixels);
	u8 *result  = 0;

	if(x < w && y < h)
	{
	    result = (u8 *)(src_32 + x + (y * w));
	}
	return(result);
}

static void
copy_pixels_to(
		u32 source_w,
		u32 source_h,
		u8 *source_pixels,
		u32 destination_w,
		u32 destination_h,
		u8 *destination_pixels)
{
	u32 w = source_w;
	u32 h = source_h;
	if(source_w > destination_w)
	{
		w = destination_w;
	}
	if(source_h > destination_h)
	{
		h = destination_h;
	}

	u32 x = 0;
	u32 y = 0;

	u32 *src_at = (u32 *)source_pixels;
	u32 *dst_at = (u32 *)destination_pixels;
	while(y < h)
	{
		while(x < w)
		{
			*(dst_at + x + (y * destination_w)) = *(src_at + x + (y * source_w));
			x++;
		}
		y++;
		x = 0;
	}

}

/*
   line
y = (x-x0) * (y1-y0) / (x1-x0) + y0.
   */

/*
   takes an image (usually should be of two colors)
   and converts it to a sdf image.
   "on_pixel" is used to specify which color should be considered as
   the pixel to take distance from

   This algorithm is based on the paper written by 
   Felzenszwalb and Huttenlocher to generate signed distance
   images.
   This uses an array of f32 (floating point values) instead of
   booleans to specify the strength of each pixel.

*/

inline f32
image_intersect_parabolas(
		f32 p_x0, f32 p_y0,
		f32 p_x1, f32 p_y1)
{
	f32 result = ((p_y0 + (p_x0 * p_x0)) - (p_y1 + (p_x1 * p_x1))) / (2 * p_x0 - 2 * p_x1);
	return(result);
}

//find lowest hull of parabolas
inline void
image_sdf_fill_squared_distances(
		        memory_area *area,
				f32 *row_or_column,
				f32 *sq_distances,
				u32 w_or_h)
{
	i32 *v = memory_area_clear_and_push_array(area, i32, w_or_h);
	f32 *z = memory_area_clear_and_push_array(area, f32, w_or_h + 1);
	i32 k = 0;
	v[0] = 0;
	z[0] = -sdf_infinite;
	z[1] = sdf_infinite;

	//p_y0 = sq_distances[q]
	//p_x0 = q
	//p_y1 = sq_distances[v[k]]
	//p_x1 = v[k]:
	for(u32 q = 1;
			q < w_or_h;
			q++)
	{
	    f32 p_x0 = (f32)q;
	    f32 p_y0 = row_or_column[q];

	    f32 p_x1 = (f32)v[k];
	    f32 p_y1 = row_or_column[v[k]];

		f32 s = image_intersect_parabolas(p_x0, p_y0, p_x1, p_y1);
		while(s <= z[k])
		{
			k--;
			Assert(k >= 0);
	        p_x1 = (f32)v[k];
	        p_y1 = row_or_column[v[k]];
			s = image_intersect_parabolas(p_x0, p_y0, p_x1, p_y1);
		}
		k++;
		v[k] = q;
		z[k] = s;
		z[k + 1] = sdf_infinite;

	}
	k = 0;
	for(i32 q = 0;
			q < (i32)w_or_h;
			q++)
	{
		while(z[k + 1] < q)
		{
			k++;
		}
		i32 dx = (q - v[k]);
		sq_distances[q] = (dx * dx) + row_or_column[v[k]];
	}
}

static u32
image_fill_sdf_deltas(
		memory_area *area,
		image_data *image,
		f32 *EDT_f32)
{
	u32 success = 0;
	success = 1;

	temporary_area temporary_rows_and_columns_area = temporary_area_begin(area);

	u32 row_or_column_max = image->width > image->height ? image->width : image->height;

	f32 *f = memory_area_push_array
		(area,
		 f32,
		 row_or_column_max);
	f32 *squared_distances = memory_area_clear_and_push_array(
			area,
			f32,
			row_or_column_max);


	//vertical pass
	for(u32 x = 0;
			x < image->width;
			x++)
	{
		//linearly fill column data
		for(u32 y = 0; y < image->height; y++)
		{
			f[y] = EDT_f32[x + (y * image->width)];
		}

		image_sdf_fill_squared_distances(
				area,
				f,
				squared_distances,
				image->height);

		for(u32 y = 0; y < image->height; y++)
		{
			EDT_f32[x + (y * image->width)] = squared_distances[y];
		}


	}
	//horizontal pass
    for(u32 y = 0;
		    y < image->height;
			y++)
	{
		for(u32 x = 0; x < image->width; x++)
		{
			f[x] = EDT_f32[x + (y * image->width)];
		}
		image_sdf_fill_squared_distances(
				area,
				f,
				squared_distances,
				image->width);

		for(u32 x = 0; x < image->width; x++)
		{
			EDT_f32[x + (y * image->width)] = squared_distances[x];
		}
	}

	temporary_area_end(&temporary_rows_and_columns_area);

	return(success);
}

static u32
image_convert_to_sdf(
		memory_area *area,
		image_data *image,
		u32 on_pixel)
{
	u32 success = 0;
	if(image)
	{
		success = 1;
	    f32 *EDT_f32 = memory_area_push_array(
				area,
			    f32,
			    image->width * image->height * image->bpp);

		//look for the "on" pixel
		u32 *pixels_32 = (u32 *)image->pixels;
		for(u32 x = 0;
				x < image->width;
				x++)
		{
			for(u32 y = 0;
					y < image->height;
					y++)
			{
				u32 row_at      = (x + (y * image->width));
				u32 pixel_color = *(pixels_32 + x + (y * image->width));
				if(pixel_color == on_pixel)
				{
					EDT_f32[row_at] = 0;
				}
				else
				{
					EDT_f32[row_at] = sdf_infinite;
				}
			}
		}
		//process the data horizontally and vertically
	    image_fill_sdf_deltas(area, image, EDT_f32);


		//find the minimum and maximum values on the edt array
		f32 min_edt = 0;
		f32 max_edt = 0;
    	//take square roots
        for(u32 y = 0;
    		    y < image->height;
    			y++)
    	{
    		for(u32 x = 0; x < image->width; x++)
    		{
    			EDT_f32[x + (y * image->width)] = sqrtf(EDT_f32[x + (y * image->width)]);

				f32 edt_value = EDT_f32[x + (y * image->width)];
				if(edt_value < min_edt)
				{
					min_edt = edt_value;
				}
				if(edt_value > max_edt)
				{
					max_edt = edt_value;
				}
    		}
    	}

#if 1
		u32 max_pixel = 255;
		f32 scale = 10;//255 / (max_edt - min_edt);
		f32 weigth = 0.5f;

        for(u32 y = 0;
				y < image->height;
				y++)
		{
			for(u32 x = 0;
					x < image->width;
					x++)
			{
				f32 edt_at = *(EDT_f32 + x + (y * image->width));
				vec4 final_color = {0};
				f32 edt_v = (edt_at - min_edt);
				final_color.x = 255 - (f32)(scale * edt_v);
				final_color.y = 255 - (f32)(scale * edt_v);
				final_color.z = 255 - (f32)(scale * edt_v);
				final_color.w = 255 - (f32)(scale * edt_v);

				final_color.x = final_color.x < 0 ? 0 : final_color.x > 255 ? 255 : final_color.x;
				final_color.y = final_color.y < 0 ? 0 : final_color.y > 255 ? 255 : final_color.y;
				final_color.z = final_color.z < 0 ? 0 : final_color.z > 255 ? 255 : final_color.z;
				final_color.w = final_color.w < 0 ? 0 : final_color.w > 255 ? 255 : final_color.w;
				*(pixels_32 + x + (y * image->width)) = RGBAPack(final_color);
			}
		}
#else
		u32 max_pixel = 255;
		f32 scale = max_pixel / (max_edt - min_edt);
        for(u32 y = 0;
				y < image->height;
				y++)
		{
			for(u32 x = 0;
					x < image->width;
					x++)
			{
				u32 pixel_at    = *(pixels_32 + x + (y * image->width));
				u32 pixel_color = (u32)(u8)(scale * (pixel_at - min_edt));
				*pixels_32 = (pixel_color < 0 ? 0 : pixel_color);
			}
		}
#endif
	}

	return(success);
}

typedef struct{
	i32 x;
	i32 y;
}sdf_point;

inline sdf_point
image_get_sdf_point_from_grid(
		sdf_point *grid,
		i32 x,
		i32 y,
		u32 grid_w,
		u32 grid_h)
{
	if(x < 0)
	{
		x = 0;
	}
	else if(x == grid_w)
	{
		x = grid_w - 1;
	}

	if(y < 0)
	{
		y = 0;
	}
	else if(y == grid_h)
	{
		y = grid_h - 1;
	}

	sdf_point result = *(grid + x + (y * grid_w));

	return(result);
}

inline sdf_point
image_get_closest_sdf_point(
		sdf_point *grid,
		u32 x,
		u32 y,
		i32 off_x,
		i32 off_y,
		u32 grid_width,
		u32 grid_height)
{
	sdf_point p0 = image_get_sdf_point_from_grid(grid, x, y, grid_width, grid_height); 
	sdf_point p1 = image_get_sdf_point_from_grid(grid, (x + off_x), (y + off_y), grid_width, grid_height); ;
	p1.x += off_x;
	p1.y += off_y;

	i32 d0 = (p0.x * p0.x + p0.y * p0.y);
	i32 d1 = (p1.x * p1.x + p1.y * p1.y);
	if(d0 < d1)
	{
		return(p0);
	}
	else
	{
		return(p1);
	}
}

inline void
image_generate_grid_sdf(
		sdf_point *grid,
		u32 image_width,
		u32 image_height)
{

	//pass 0
	for(u32 y = 0;
			y < image_height;
			y++)
	{
		for(u32 x = 0;
				x < image_width;
				x++)
		{
			sdf_point p = *(grid + x + (y * image_width));
			p = image_get_closest_sdf_point(
					grid, x, y, -1, 0, image_width, image_height);
			p = image_get_closest_sdf_point(
					grid, x, y, 0, -1, image_width, image_height);
			p = image_get_closest_sdf_point(
					grid, x, y, -1, -1, image_width, image_height);
			p = image_get_closest_sdf_point(
					grid, x, y, 1, -1, image_width, image_height);

			*(grid + x + (y * image_width)) = p;

		}
		for(i32 x = image_width -1;
				x >= 0;
				x--)
		{
			sdf_point p = *(grid + x + (y * image_width));
			p = image_get_closest_sdf_point(
					grid, x, y, 1, 0, image_width, image_height);

			*(grid + x + (y * image_width)) = p;
		}
	}
	//second pass
	for(i32 y = image_height - 1;
			y >= 0;
			y--)
	{
		for(i32 x = image_width - 1;
				x >= 0;
				x--)
		{
			sdf_point p = *(grid + x + (y * image_width));
			p = image_get_closest_sdf_point(
					grid, x, y, 1, 0, image_width, image_height);
			p = image_get_closest_sdf_point(
					grid, x, y, 0, 1, image_width, image_height);
			p = image_get_closest_sdf_point(
					grid, x, y, -1, 1, image_width, image_height);
			p = image_get_closest_sdf_point(
					grid, x, y, 1, 1, image_width, image_height);

			*(grid + x + (y * image_width)) = p;

		}
		for(u32 x = 0;
				x < image_width;
				x++)
		{
			sdf_point p = *(grid + x + (y * image_width));
			p = image_get_closest_sdf_point(
					grid, x, y, -1, 0, image_width, image_height);

			*(grid + x + (y * image_width)) = p;
		}
	}
}

static u32
image_convert_to_sdf_8ssedt(
		memory_area *area,
		image_data *image,
		u32 on_pixel)
{

	u32 success = 0;
	temporary_area temporary_grids_area = temporary_area_begin(area);

	u32 img_size = image->width * image->height;
	sdf_point *grid0 = memory_area_push_array(area, sdf_point, img_size);
	sdf_point *grid1 = memory_area_push_array(area, sdf_point, img_size);

	u32 *pixels_32 = (u32 *)image->pixels;
	for(u32 y = 0;
			y < image->height;
			y++)
	{
		for(u32 x = 0;
				x < image->width;
				x++)
		{
			u32 pixel_color = *(pixels_32 + x + (y * image->width));
			sdf_point *p0 = (grid0 + x + (y * image->width));
			sdf_point *p1 = (grid1 + x + (y * image->width));
			if(pixel_color == on_pixel)
			{
				p0->x = 0;
				p0->y = 0;
				p1->x = 9999;
				p1->y = 9999;
			}
			else
			{
				p0->x = 9999;
				p0->y = 9999;
				p1->x = 0;
				p1->y = 0;
			}
		}
	}

    image_generate_grid_sdf(
    		grid0,
    		image->width,
    		image->height);

    image_generate_grid_sdf(
    		grid1,
    		image->width,
    		image->height);

	for(u32 y = 0;
			y < image->height;
			y++)
	{
		for(u32 x = 0;
				x < image->width;
				x++)
		{
			sdf_point *p0 = (grid0 + x + (y * image->width));
			sdf_point *p1 = (grid1 + x + (y * image->width));
			i32 d0 = (i32)sqrtf(((f32)p0->x * p0->x) + ((f32)p0->y * p0->y));
			i32 d1 = (i32)sqrtf(((f32)p1->x * p1->x) + ((f32)p1->y * p1->y));
			i32 d = d0 - d1;

			i32 final_pixel = 255 - (d * 3);
			final_pixel = final_pixel < 0 ? 0 : final_pixel > 255 ? 255 : final_pixel;

			vec4 final_pixel_v4 = 
			{(f32)final_pixel,
			 (f32)final_pixel,
			 (f32)final_pixel,
			 (f32)final_pixel};

			final_pixel = RGBAPack(final_pixel_v4);

			*(pixels_32 + x + (y * image->width)) = final_pixel;
		}
	}



	temporary_area_end(&temporary_grids_area);
	return(success);
}
