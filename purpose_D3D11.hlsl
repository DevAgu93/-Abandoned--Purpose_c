// purpose_D3D11.c
// fwidth combines ddx and ddx
// ddx and ddx are pixel steps in respect of the resolution (not the actual pixel coordinates, but their difference)
// smooth step is a returns a linear delta time between two Min and Max values.
// step is near, so only returns 1

u8 VertexShaderCode[] = SHADER(
          //Vertex shader data
          cbuffer CameraShaderBuffer 
          {
              float4x4 WVP;
          };
          struct VTOut
          {
              float4 position : SV_POSITION;
              float2 texcoord : TEXCOORD;
              uint textindex : BLENDINDICES;
              float4 color : COLOR;
          };
          
          VTOut VShaderT(float3 position : POSITION,
                         float2 intext : TEXCOORD,
                         uint4 InColor : COLOR,
                         uint IntTextureIndex : BLENDINDICES)
                      
          {
              VTOut output;
          //This sorts by column major by default.
              output.position  = mul(float4(position, 1), WVP);
            //  output.position  = float4(position, 1);
          	output.texcoord  = intext;
              output.textindex = IntTextureIndex;

              output.color = InColor;
              output.color /= 255;
          
              return output;
          }
          );


u8 PixelShaderCode[] = SHADER(
          //============Direct3d 10+ pixel shader code
          \n#define PI 3.14159265359f\n
          \n#define PI_half 1.57079632679f\n
          \n#define PI_half_taylor 1.00452485553f\n
          \n#define Alpha 0.0f\n

          Texture2DArray texturearray;
          Texture2D DepthPeel : register(t1);
          SamplerState samplerState;
          sampler sam;
          
          float4 Filter_1(Texture2DArray OutTexture,
                               float2 outUv : TEXCOORD,
                               uint OutTIndex : BLENDINDICES) : SV_TARGET
          {
              float2 TextureSize = float2(512.0f, 512.0f);
              float2 pixel = 1.0f / TextureSize;
              float2 uv_scaled = outUv * TextureSize;
              float2 uv = floor(uv_scaled);

              float2 fddxy = fwidth(uv_scaled);
              float2 a = Alpha * fddxy;
              float2 uv_frac = frac(uv_scaled);
              float2 x = clamp(0.5f / a * uv_frac, 0.0f,0.5f) + 
                         clamp(0.5f / a * (uv_frac - 1.0f) + 0.5f, 0.0f, 0.5f);
              uv += x;
              uv *= pixel;

              float3 uvi = float3(uv, OutTIndex);
              float4 c = OutTexture.Sample(samplerState, uvi);
              return c;
          }
          float4 Filter_2(Texture2DArray OutTexture,
                             float2 outUv : TEXCOORD,
                             uint OutTIndex : BLENDINDICES) : SV_TARGET
          {
              float2 TextureSize = float2(512.0f, 512.0f);
              float2 pixel = 1.0f / TextureSize;
              float2 uv_scaled = outUv * TextureSize;

              float2 uv = floor(uv_scaled + 0.5f);
              float2 fr = frac(uv_scaled + 0.5f);
              float2 aa = fwidth(uv_scaled) * SamplerWidth23 * 0.5f;

              fr = smoothstep(0.5f - aa, 0.5f + aa, fr);
              uv = (uv + fr - 0.5f);
              uv *= pixel;

              float3 uvi = float3(uv, OutTIndex);
              float4 c = OutTexture.Sample(samplerState, uvi);
              return c;
          }
          float4 Filter_3(Texture2DArray OutTexture,
                             float2 outUv : TEXCOORD,
                             uint OutTIndex : BLENDINDICES) : SV_TARGET
          {
              float2 TextureSize = float2(512.0f, 512.0f);
              float2 pixel = 1.0f / TextureSize;
              float2 uv_scaled = outUv * TextureSize;

              float2 uv = floor(uv_scaled  + 1);
              uv += clamp((uv_scaled - uv) / fwidth(uv_scaled) / SamplerWidth23, -0.5f, 0.5f);
              uv *= pixel;

              float3 uvi = float3(uv, OutTIndex);
              float4 c = OutTexture.Sample(samplerState, uvi);
              return c;
          }

          float4 Filter_4(Texture2DArray OutTexture,
                             float2 outUv : TEXCOORD,
                             uint OutTIndex : BLENDINDICES) : SV_TARGET
          {
              
              float2 TextureSize = float2(512.0f, 512.0f);

              float2 pixel = 1.0f / TextureSize;
			  //Scale the given texture coordinate
              float2 uv_scaled = outUv * TextureSize;

              float2 uv = uv_scaled - 0.5f;
              float2 uv_floor = floor(uv);
              float2 phase = uv - uv_floor;

              float2 SampleDelta2 = max(fwidth(uv_scaled) * SamplerMod4, 1.0f / 256);
              min16float2 shift = 0.5f + 0.5f * sin(PI_half * clamp((phase - 0.5f) / min(SampleDelta2, 0.5f), -1.0f, 1.0f));
              float2 uv_final  = (uv_floor + 0.5f + shift) * pixel;

              float3 uvi = float3(uv_final, OutTIndex);
              float4 c = OutTexture.Sample(samplerState, uvi);
              return c;
          }
		  \n/*Nothing or near*/\n
          float4 Filter_0(Texture2DArray OutTexture,
                               float2 outUv : TEXCOORD,
                               uint OutTIndex : BLENDINDICES) : SV_TARGET
          {
              float3 uvi = float3(outUv, OutTIndex);
              float4 c = OutTexture.Sample(samplerState, uvi);
              return c;
          }

          float4 Filter_n(Texture2DArray OutTexture,
                               float2 outUv : TEXCOORD,
                               uint OutTIndex : BLENDINDICES) : SV_TARGET
          {
			  float smoothing = 1.0f / 16.0f;
			  float width = 0.04f;
              float3 uvi = float3(outUv, OutTIndex);
              float4 color = OutTexture.Sample(samplerState, uvi);
			  float c = color.w; 
			  float d = 1.0f - c;
			  float a = 1.0f - smoothstep(width, width + 0.1f, d);
			  color.a = a;

              return color;
          }

          float4 MainPixelShader(float4 position : SV_POSITION,
                                 float2 outUv : TEXCOORD,
                                 uint OutTIndex : BLENDINDICES,
                                 float4 OutColor : COLOR) : SV_TARGET
          {
              
              //SELECTED PIXEL SAMPLER IN MACRO
              float4 c = SAMPLER(texturearray, outUv, OutTIndex);;

              //float3 uvi = float3(outUv, OutTIndex);
              //float4 cdos = tex3D(sam, uvi);
              //OutColor = float4(1.0f, 0.0f, 0.0f, 1.0f);
			  OutColor.rgb *= OutColor.w;
              return c * OutColor;
          }

          float4 PeelShader(float4 position : SV_POSITION,
                            float2 outUv : TEXCOORD,
                            uint OutTIndex : BLENDINDICES,
                            float4 OutColor : COLOR) : SV_TARGET
          {
              
              //What is already there
              float TexelDepth = DepthPeel.Load(int3(position.xy, 0)).r;
              //The screen space I'm about to use
              float PixelDepth = position.z;
              if(PixelDepth <= TexelDepth)
              {
                 discard; 
              }
              float3 uvi = float3(outUv, OutTIndex);
              float4 c = SAMPLER(texturearray, outUv, OutTIndex); 
              if(c.a == 0)
              {
                  discard;
              }

			  OutColor.rgb *= OutColor.w;
              return c * OutColor;
          }

          float4 ps_Smooth(float4 position : SV_POSITION,
                           float2 outUv : TEXCOORD,
                           uint OutTIndex : BLENDINDICES,
                           float4 OutColor : COLOR) : SV_TARGET
          {
              
              //SELECTED PIXEL SAMPLER IN MACRO

              float3 uvi = float3(outUv, OutTIndex);
              float4 c   = texturearray.Sample(samplerState, uvi);

              //float3 uvi = float3(outUv, OutTIndex);
              //float4 cdos = tex3D(sam, uvi);
              //OutColor = float4(1.0f, 0.0f, 0.0f, 1.0f);

			  //float sX = step(outUv.y, 0.5f);
              float fw = fwidth(outUv) * 9;
              float dy = abs(ddy(outUv.y)) * 9;

              float centerDistance = distance(outUv, float2(0.5f, 0.5f));
			  float yDistance = abs(outUv.y - 0.5f);
			  float sX = smoothstep(0.0f, 0.0f + dy, yDistance);
			  if(1)
			  {
				  float w = OutColor.a;
				  OutColor *= sX;
				  //OutColor.a = w;
			  }


			  OutColor.rgb *= OutColor.w;

              return c * OutColor;
          }
    );



    char PixelCompositeShaderCode[] = SHADER(
             Texture2D Layer1 : register(t1);
             Texture2D Layer2 : register(t2);
             Texture2D Layer3 : register(t3);
             Texture2D Layer4 : register(t4);
            SamplerState samplerState;

    float4 CompositeShader(float4 position : SV_POSITION,
                           float2 texcoord : TEXCOORD,
                           uint textindex : BLENDINDICES) : SV_TARGET
    {
       //square both colors if srgb

      float4 l0c = Layer1.Sample(samplerState, texcoord);
      float4 l1c = Layer2.Sample(samplerState, texcoord);
      float4 l2c = Layer3.Sample(samplerState, texcoord);
      float4 l3c = Layer4.Sample(samplerState, texcoord);
      float4 c = 0;
      c.rgb = l3c.rgb;
      //From the last shader to the first
      c.rgb = l2c.rgb + (1.0f - l2c.a) * c.rgb;
      c.rgb = l1c.rgb + (1.0f - l1c.a) * c.rgb;
      c.rgb = l0c.rgb + (1.0f - l0c.a) * c.rgb;

      c = clamp(c, 0, 1);
      //square root if srgb
      return c;
    }
    );
