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
          struct vt_out
          {
              float4 position : SV_POSITION;
              float2 texcoord : TEXCOORD;
              uint textindex : BLENDINDICES;
              float4 color : COLOR;
          };
          
          2t_out VShaderT(float2 position : POSITION,
                         float2 in_texture_uvs : TEXCOORD,
                         uint4 in_color : COLOR,
                         uint in_texture_index : BLENDINDICES)
                      
		  {
		      vt_out output;
		      //This sorts by column major by default.
		      output.position  = mul(float4(position, 1), WVP);
		      //  output.position  = float4(position, 1);
		      output.texcoord  = in_texture_uvs;
		      output.textindex = in_texture_index;

		      output.color = in_color;
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
          
		  \n/*Nothing or near*/\n
          float4 Filter_0(Texture2DArray OutTexture,
                               float2 outUv : TEXCOORD,
                               uint OutTIndex : BLENDINDICES) : SV_TARGET
          {
              float3 uvi = float3(outUv, OutTIndex);
              float4 c = OutTexture.Sample(samplerState, uvi);
              return c;
          }
