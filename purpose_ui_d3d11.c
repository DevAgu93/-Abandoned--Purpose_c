#define _UI_MAXIMUM_QUAD_DRAWS_IN_INDEX_BUFFER ((1 << 14) - 1)
#define _UI_MAXINDICES (MAXIMUM_QUAD_DRAWS_IN_INDEX_BUFFER * 4) 

typedef struct{
	vec2 location;
	vec2 uv;
	u32 color;
	u32 texture;
}ui_render_vertex;

typedef struct{
	ID3D11Device *device;
	ID3D11DeviceContext *device_context;
    ID3D11Buffer *vertex_buffer;
    ID3D11Buffer *index_buffer;

    ID3D11VertexShader *vertex_shader;    // the vertex shader
    ID3D11PixelShader *pixel_shader;
}ui_d3d11;

#define UI_SHADER(...) #__VA_ARGS__

static inline ui_d3d11
ui_d3d11_init(
	memory_area *render_area,
	ID3D11Device *device,
	ID3D11DeviceContext *device_context
	)
{
	ui_d3d11 ui_d3d11 = {0};
	HRESULT hresult = 0;

	u32 quad_draw_count = 40000;
	u32 quad_draw_size = quad_draw_count * (sizeof(ui_render_vertex) * 4);
	u32 index_buffer_max = quad_draw_count * 6;
	u32 index_buffer_size = index_buffer_max * sizeof(u32);
	//Initialize shader
	//allocate vertex buffer
	void *vertex_buffer_memory = memory_area_push_size(render_area, quad_draw_size);
	//allocate index buffer
	u32 *index_buffer_memory = memory_area_push_array(
			render_area, u32, index_buffer_max);

   u32 index_value = 0;
	//fill index buffer
   for(u32 i = 0; i < index_buffer_max; i += 6)
   {
       index_buffer_memory[i]     = index_value; 
       index_buffer_memory[i + 1] = index_value + 1; 
       index_buffer_memory[i + 2] = index_value + 2; 

       index_buffer_memory[i + 3] = index_value; 
       index_buffer_memory[i + 4] = index_value + 2; 
       index_buffer_memory[i + 5] = index_value + 3; 
       index_value += 4;
   }

   //create vertex and index buffers
   D3D11_BUFFER_DESC vertex_buffer_desc = {0};
   D3D11_SUBRESOURCE_DATA vertex_buffer_data = {0};
   D3D11_SUBRESOURCE_DATA index_buffer_data  = {0};

   vertex_buffer_data.pSysMem = vertex_buffer_memory; 

   vertex_buffer_desc.ByteWidth = quad_draw_size; 
   vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
   vertex_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
   vertex_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;

   //fill index desc and subresource data
   D3D11_BUFFER_DESC indexdesc = {0};
   index_buffer_data.pSysMem  = index_buffer_memory;

   indexdesc.ByteWidth = index_buffer_size;
   indexdesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
   indexdesc.CPUAccessFlags = 0;// D3D11_CPU_ACCESS_WRITE;
   indexdesc.Usage = D3D11_USAGE_IMMUTABLE;

   //Creates and fills vertex buffer
   hresult = ID3D11Device_CreateBuffer(device,
		   &vertex_buffer_desc,
		   &vertex_buffer_data,
		   &ui_d3d11.vertex_buffer);
   Assert(hresult == S_OK);
   //create index buffer
   hresult = ID3D11Device_CreateBuffer(device,
		   &indexdesc,
		   &index_buffer_data,
		   &ui_d3d11.index_buffer);
   Assert(hresult == S_OK);






   D3D11_INPUT_ELEMENT_DESC VertexSpriteData[] = 
   {
       {"POSITION"    , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
       {"TEXCOORD"    , 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
       {"COLOR"       , 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
       {"BLENDINDICES", 0, DXGI_FORMAT_R32_UINT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
   };
   ID3DBlob *vertex_shader_blob;
   ID3DBlob *pixel_shader_blob;
   ID3DBlob *error_msg;

   u8 ui_vertex_shader[] = {
          "//Vertex shader data\
          cbuffer camera_shader\
          {\
              float4x4 WVP;\
          };\
          struct vt_out\
          {\
              float4 position : SV_POSITION;\
              float2 texcoord : TEXCOORD;\
              uint textindex : BLENDINDICES;\
              float4 color : COLOR;\
          };\
          vt_out v_main(float2 position : POSITION,\
                         float2 in_texture_uvs : TEXCOORD,\
                         uint4 in_color : COLOR,\
                         uint in_texture_index : BLENDINDICES)\
		  {\
		      vt_out output;\
		      //This sorts by column major by default.\
		      output.position  = mul(float4(position, 1), WVP);\
		      //  output.position  = float4(position, 1);\
		      output.texcoord  = in_texture_uvs;\
		      output.textindex = in_texture_index;\
\
		      output.color = in_color;\
		      output.color /= 255;\
\
		      return output;\
		  }"
   };

   u8 ui_pixel_shader[] = {
          "//Vertex shader data\
          cbuffer camera_shader\
          {\
              float4x4 WVP;\
          };\
          struct vt_out\
          {\
              float4 position : SV_POSITION;\
              float2 texcoord : TEXCOORD;\
              uint textindex : BLENDINDICES;\
              float4 color : COLOR;\
          };\
          float4 Filter_0(Texture2DArray OutTexture,\
                               float2 outUv : TEXCOORD,\
                               uint OutTIndex : BLENDINDICES) : SV_TARGET\
          {\
              float3 uvi = float3(outUv, OutTIndex);\
              float4 c = OutTexture.Sample(samplerState, uvi);\
              return c;\
          }\
          float4 ps_main(float4 position : SV_POSITION,\
                                 float2 outUv : TEXCOORD,\
                                 uint OutTIndex : BLENDINDICES,\
                                 float4 OutColor : COLOR) : SV_TARGET\
          {\
              \
              //SELECTED PIXEL SAMPLER IN MACRO\
              float4 c = SAMPLER(texturearray, outUv, OutTIndex);;\
\
              //float3 uvi = float3(outUv, OutTIndex);\
              //float4 cdos = tex3D(sam, uvi);\
              //OutColor = float4(1.0f, 0.0f, 0.0f, 1.0f);\
			  OutColor.rgb *= OutColor.w;\
              return c * OutColor;\
          }"
   };

	hresult = D3DCompile(ui_vertex_shader,
			sizeof(ui_vertex_shader),
			0,
			0,
			0,
			"v_main",
			"vs_4_0",
			D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,
			0,
			&vertex_shader_blob,
			&error_msg);

	hresult = D3DCompile(ui_pixel_shader,
			sizeof(ui_pixel_shader),
			0,
			0,
			0,
			"ps_main",
			"ps_4_0",
			D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,
			0,
			&pixel_shader_blob,
			&error_msg);
   Assert(hresult == S_OK);


	return(ui_d3d11);
}

static inline void
ui_d3d11_begin()
{
}
static inline void
ui_d3d11_end()
{
}
