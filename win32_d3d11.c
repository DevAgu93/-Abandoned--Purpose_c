#include "purpose_crt.h"

#include "global_definitions.h"
#include "purpose_global.h"
#include "purpose_math.h"
#include "purpose_memory.h"
#include "purpose_render.h"
#include "agu_image.c"
//#include "purpose_crt.h"
//#include <global_definitions.h>
//#include <gmmath.h>
//#include <gm_memory.h>
//#include <gm_render.h>
//#include "purpose_image.c"

/*
Notes:
For displaying, the 0-index frame buffer in frame_buffers is the default displaying buffer
a depth buffer can optionally be used.
If using depth peeling, 4 extra frame buffers alongside depth buffers are created with
d3d11_create_frame_buffer and d3d_create_depth_buffer and allocated in both frame_buffers (after the default display buffer)
and the depth_buffers.

ID3D11DeviceContext_DrawIndexed accepts:
[0]a pointer to the device context,
[1]an integer indicating how many indices from the index buffer to draw
[2]an integer indicating from where to start reading in the
index buffer
[3]An integer used as index to indicate where to start reading
from the vertex buffer

Depth peeling:
depth peeling render commands are divided into groups. When render commands
without the depth peel flag are being processed after one with said flag, this
render command is saved for last when the depth peeling process is finished.
TLDR: First, the depth peeling render commands are drawn, then the ones who don't use it.

*/

#define COBJMACROS
#define CINTERFACE
#define SAMPLING 1

#include <d3d11.h>
//#include <d3dcompiler.h>
#define D3DCOMPILE_DEBUG                                (1 << 0)
#define D3DCOMPILE_SKIP_VALIDATION                      (1 << 1)
#define D3DCOMPILE_SKIP_OPTIMIZATION                    (1 << 2)
#define D3DCOMPILE_PACK_MATRIX_ROW_MAJOR                (1 << 3)
#define D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR             (1 << 4)
#define D3DCOMPILE_PARTIAL_PRECISION                    (1 << 5)
#define D3DCOMPILE_FORCE_VS_SOFTWARE_NO_OPT             (1 << 6)
#define D3DCOMPILE_FORCE_PS_SOFTWARE_NO_OPT             (1 << 7)
#define D3DCOMPILE_NO_PRESHADER                         (1 << 8)
#define D3DCOMPILE_AVOID_FLOW_CONTROL                   (1 << 9)
#define D3DCOMPILE_PREFER_FLOW_CONTROL                  (1 << 10)
#define D3DCOMPILE_ENABLE_STRICTNESS                    (1 << 11)
#define D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY       (1 << 12)
#define D3DCOMPILE_IEEE_STRICTNESS                      (1 << 13)
#define D3DCOMPILE_OPTIMIZATION_LEVEL0                  (1 << 14)
#define D3DCOMPILE_OPTIMIZATION_LEVEL1                  0
#define D3DCOMPILE_OPTIMIZATION_LEVEL2                  ((1 << 14) | (1 << 15))
#define D3DCOMPILE_OPTIMIZATION_LEVEL3                  (1 << 15)
#define D3DCOMPILE_RESERVED16                           (1 << 16)
#define D3DCOMPILE_RESERVED17                           (1 << 17)
#define D3DCOMPILE_WARNINGS_ARE_ERRORS                  (1 << 18)
#define D3DCOMPILE_RESOURCES_MAY_ALIAS                  (1 << 19)
#define D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES   (1 << 20)
#define D3DCOMPILE_ALL_RESOURCES_BOUND                  (1 << 21)
#define D3DCOMPILE_DEBUG_NAME_FOR_SOURCE                (1 << 22)
#define D3DCOMPILE_DEBUG_NAME_FOR_BINARY                (1 << 23)


HRESULT WINAPI
D3DCompile(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData,
           _In_ SIZE_T SrcDataSize,
           _In_opt_ LPCSTR pSourceName,
           _In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* pDefines,
           _In_opt_ ID3DInclude* pInclude,
           _In_opt_ LPCSTR pEntrypoint,
           _In_ LPCSTR pTarget,
           _In_ UINT Flags1,
           _In_ UINT Flags2,
           _Out_ ID3DBlob** ppCode,
           _Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorMsgs);

//
// IDXGIFactory
//
DEFINE_GUID(IID_IDXGIFactory,0x7b7166ec,0x21c7,0x44ae,0xb2,0x1a,0xc9,0xae,0x32,0x1a,0xe3,0x69);

HRESULT WINAPI CreateDXGIFactory(REFIID riid, _COM_Outptr_ void **ppFactory);
#define IDXGIFactory_CreateSwapChain(This,pDevice,pDesc,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChain(This,pDevice,pDesc,ppSwapChain) ) 
























//
//
//

#define d3dBlobBufferPointer(shadcode) shadcode->lpVtbl->GetBufferPointer(shadcode)
#define d3dBlobBufferSize(shadcode) shadcode->lpVtbl->GetBufferSize(shadcode)
#define d3d_vs_SetShader(devicecontext, vertexshader, poutclassinstances, classinstancescount) devicecontext->lpVtbl->VSSetShader(devicecontext, vertexshader, poutclassinstances, classinstancescount )
#define d3d_ps_CreateShader(device, shadcode, shadsize, classes, pixelshaderout) device->lpVtbl->CreatePixelShader(device, shadcode, shadsize, classes, pixelshaderout)
#define d3d_ps_SetShader(devicecontext, pixelshader, poutclassinstances, classinstancescount) devicecontext->lpVtbl->PSSetShader(devicecontext, pixelshader, poutclassinstances, classinstancescount )
#define d3dRelease(interface) interface->lpVtbl->Release(interface)
#define MAXIMUM_QUAD_DRAWS_IN_INDEX_BUFFER ((1 << 14) - 1)
#define MAXINDICES (MAXIMUM_QUAD_DRAWS_IN_INDEX_BUFFER * 4) 

//#include <gmui_d3d11.c>


typedef struct{
	//to avoid compiler warnings, this
	//D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT is treated as void*
	//use d3d11_create_frame_buffer to create a one of these. It
	//can be used for both frame or depth buffering.
    void *RenderHandle;
    ID3D11ShaderResourceView *ResourceHandle;
}d3d_buffer;

typedef struct 
{
    platform_renderer header;
    ID3D11Device           *device;
    ID3D11DeviceContext    *devicecontext;
    IDXGISwapChain         *swapchain; 

	u16 frame_buffer_count;
	u16 frame_buffer_max;
    d3d_buffer *frame_buffers;
    d3d_buffer *depth_buffers;
	u16 last_frame_buffer_index;
    u16 current_frame_buffer_index;

    ID3D11VertexShader *vsDefault;    // the vertex shader

    ID3D11PixelShader *psDefault;
    ID3D11PixelShader *ps_peeling_shader;
    ID3D11PixelShader *psCompositeShader;
    ID3D11PixelShader *psSmoothShader;

    ID3D11Buffer *camerashaderbuffer;
    ID3D11Texture2D *depthStencil;

    ID3D11DepthStencilState *DefaultDepthState;
    ID3D11SamplerState *samplerstate;
    ID3D11BlendState *blendstate;

    ID3D11Buffer *vertex_buffer;
    ID3D11Buffer *index_buffer;

    u32 texture_array_w;
    u32 texture_array_h;
    u32 mip_levels;

	u32 depth_peel_layer_count;
    ID3D11ShaderResourceView *textureArrayView;
}d3d_device;
//read shader errors on assertion
char*
ShaderError(ID3DBlob *shadercode)
{
    return (char *)shadercode->lpVtbl->GetBufferPointer(shadercode);
}
//IA: Input Assembler OM: Output Merger SO:Stream output


static inline D3D11_TEXTURE_ADDRESS_MODE
platform_uv_adress_to_d3d11(renderer_uv_adress adress)
{
	D3D11_TEXTURE_ADDRESS_MODE d3d11_adress[] =
	{
		D3D11_TEXTURE_ADDRESS_WRAP,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_MIRROR,
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_TEXTURE_ADDRESS_MIRROR_ONCE
	};
    D3D11_TEXTURE_ADDRESS_MODE result = D3D11_TEXTURE_ADDRESS_WRAP;
	if(adress < 4)
	{
		result = d3d11_adress[adress];
	}
	return(result);
}
static inline D3D11_FILTER
platform_filter_to_d3d11(
		renderer_filter min_filter,
		renderer_filter max_filter,
		renderer_filter mip_filter)
{
	u32 max_f[2] = {
		0,
		0x4,
	};
	u32 mip_f[2] = {
		0,
		0x1
	};
	u32 min_f[2] = {
		0,
		0x10
	};
	min_filter = min_filter > 1 ? 1 : min_filter;
	max_filter = max_filter > 1 ? 1 : max_filter;
	mip_filter = mip_filter > 1 ? 1 : mip_filter;
	D3D11_FILTER result_filters = 
		min_f[min_filter] | max_f[max_filter] | mip_f[mip_filter];
	return(result_filters);
}

inline HRESULT 
d3d11_create_input_layout(
		ID3D11Device *device,
		D3D11_INPUT_ELEMENT_DESC *elements,
		u32 attributecount,
		ID3DBlob *shadercode,
		ID3D11InputLayout **out_inputlayout)
{
   HRESULT result = ID3D11Device_CreateInputLayout(device,
                     elements, 
                     attributecount, 
                     ID3D10Blob_GetBufferPointer(shadercode),
                     ID3D10Blob_GetBufferSize(shadercode),
                     out_inputlayout);
   return result;
}
inline u32 
d3d11_create_frame_buffer(
		d3d_device *directDevice,
		u32 frame_buffer_w,
		u32 frame_buffer_h)
{
	Assert(directDevice->frame_buffer_count < directDevice->frame_buffer_max);

	ID3D11Texture2D *texturetarget;
	ID3D11RenderTargetView *texturetargetview;
	ID3D11ShaderResourceView *texturetargetresource;

	D3D11_TEXTURE2D_DESC texturetargetdesc = {0}; 
	D3D11_RENDER_TARGET_VIEW_DESC texturetargetviewdesc = {0};
	D3D11_SHADER_RESOURCE_VIEW_DESC texturetargetresourcedesc = {0};

	texturetargetdesc.Width               = frame_buffer_w;
	texturetargetdesc.Height              = frame_buffer_h;
	texturetargetdesc.MipLevels           = 1;
	texturetargetdesc.ArraySize           = 1;
	texturetargetdesc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
	texturetargetdesc.SampleDesc.Count    = SAMPLING;
	texturetargetdesc.SampleDesc.Quality  = 0;
	texturetargetdesc.Usage               = D3D11_USAGE_DEFAULT;
	texturetargetdesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texturetargetdesc.CPUAccessFlags      = 0;
	texturetargetdesc.MiscFlags           = 0;

	texturetargetviewdesc.Format = texturetargetdesc.Format;
	texturetargetviewdesc.ViewDimension = SAMPLING > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS :  D3D11_RTV_DIMENSION_TEXTURE2D;
	texturetargetviewdesc.Texture2D.MipSlice = 0;

	texturetargetresourcedesc.Format = texturetargetdesc.Format;
	texturetargetresourcedesc.ViewDimension =SAMPLING > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
	texturetargetresourcedesc.Texture2D.MostDetailedMip = 0;
	texturetargetresourcedesc.Texture2D.MipLevels = 1;

	HRESULT result = ID3D11Device_CreateTexture2D(
			directDevice->device,
			&texturetargetdesc,
			0,
			&texturetarget);
	Assert(result == S_OK);
	result = ID3D11Device_CreateRenderTargetView(
			directDevice->device,
			(ID3D11Resource *)texturetarget,
			&texturetargetviewdesc,
			&texturetargetview);
	Assert(result == S_OK);
	result = ID3D11Device_CreateShaderResourceView(
			directDevice->device,
			(ID3D11Resource *)texturetarget,
			&texturetargetresourcedesc,
			&texturetargetresource);
	Assert(result == S_OK);
	d3dRelease(texturetarget);

	d3d_buffer framebuffer = 
	{
		texturetargetview,
		texturetargetresource
	};
	//skip the default frame buffer
	u32 new_frame_buffer_index = directDevice->frame_buffer_count;
	directDevice->frame_buffer_count++;
    directDevice->frame_buffers[new_frame_buffer_index] = framebuffer;
	return(new_frame_buffer_index);
}


inline void 
d3d11_create_texture_array(d3d_device *directDevice,
                      i32 width,
                      i32 height,
                      u32 mip_levels,
                      u32 capacity)
{
   ID3D11Device *device = directDevice->device;
   ID3D11DeviceContext *devicecontext = directDevice->devicecontext;

   D3D11_SHADER_RESOURCE_VIEW_DESC shaderdesc = {0};
   ID3D11Texture2D *Createdtexturearray;
   D3D11_TEXTURE2D_DESC texturearraydesc = {0};
   D3D11_SUBRESOURCE_DATA TextureArrayData = {0};


   texturearraydesc.Width = width;
   texturearraydesc.Height = height;
   texturearraydesc.MipLevels = mip_levels;
   texturearraydesc.ArraySize = capacity;
   texturearraydesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
   texturearraydesc.Usage = D3D11_USAGE_DEFAULT;
   texturearraydesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
   texturearraydesc.SampleDesc.Count = SAMPLING;
   texturearraydesc.SampleDesc.Quality = 0;

   shaderdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
   shaderdesc.ViewDimension = SAMPLING > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D11_SRV_DIMENSION_TEXTURE2DARRAY; //SRV = Shader Resource View
   shaderdesc.Texture2D.MipLevels = -1; 
   shaderdesc.Texture2DArray.ArraySize = capacity;
   shaderdesc.Texture2DArray.MostDetailedMip = 0;
   shaderdesc.Texture2DArray.MipLevels = -1; 
   shaderdesc.Texture2DArray.FirstArraySlice = 0;

   HRESULT result = ID3D11Device_CreateTexture2D(device, &texturearraydesc, 0, &Createdtexturearray);
   Assert(result == S_OK);
   result = ID3D11Device_CreateShaderResourceView(device,
                                                 (ID3D11Resource *)Createdtexturearray,
                                                 &shaderdesc,
                                                 &directDevice->textureArrayView);
   Assert(result == S_OK);
   ID3D11DeviceChild_Release(Createdtexturearray);
}
#define SHADER(...) #__VA_ARGS__
static void
d3d_InitializeShaderProgram(d3d_device *directDevice)
{

    ID3DBlob *VertexShaderBlob;
    ID3DBlob *PixelShaderBlob;
    ID3DBlob *PeelShaderBlob;
    ID3DBlob *PixelCompositeShaderBlob;
    ID3DBlob *psSmoothBlob;

    ID3DBlob *errormsg;
    //char PShaderVersion[] = 

	//I normally use samplerMod4 1.5f
    D3D_SHADER_MACRO shadermacros[] = 
    { "SamplerWidth23", "1.0f",
      "SamplerMod4", "1.5f",
      "SAMPLER", "Filter_0",
      0, 0 };

	//VertexShaderCode, PixelShaderCode are defined here.
#include "purpose_D3D11.hlsl"

   HRESULT result = D3DCompile(VertexShaderCode,
                        sizeof(VertexShaderCode),
                        0,
                        0, //defines
                        0,
                        "VShaderT",
                        "vs_4_0",
                        D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,
                        0,
                        &VertexShaderBlob,
                        &errormsg);
   Assert(result == S_OK);
  //Note(Agu):Pixel shaders
  result = D3DCompile(PixelShaderCode,
               sizeof(PixelShaderCode),
               0,
               shadermacros,
               0,
               "MainPixelShader",
               "ps_4_0",
               D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,
               0,
               &PixelShaderBlob,
               &errormsg);
   Assert(result == S_OK);

  result = D3DCompile(PixelShaderCode,
               sizeof(PixelShaderCode),
               0,
               shadermacros, //defines
               0,
               "PeelShader",
               "ps_4_0",
               D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,
               0,
               &PeelShaderBlob,
               &errormsg);
   Assert(result == S_OK);

  result = D3DCompile(PixelCompositeShaderCode,
              sizeof(PixelCompositeShaderCode),
              0,
              0, //defines
              0,
              "CompositeShader",
              "ps_4_0",
              D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,
              0,
              &PixelCompositeShaderBlob,
              &errormsg);

  result = D3DCompile(PixelShaderCode,
               sizeof(PixelShaderCode),
               0,
               shadermacros, //defines
               0,
               "ps_Smooth",
               "ps_4_0",
               D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,
               0,
               &psSmoothBlob,
               &errormsg);
   Assert(result == S_OK);


   result = ID3D11Device_CreateVertexShader(directDevice->device, 
                                            ID3D10Blob_GetBufferPointer(VertexShaderBlob),
                                            ID3D10Blob_GetBufferSize(VertexShaderBlob),0, &directDevice->vsDefault);
   Assert(result == S_OK);

   result = d3d_ps_CreateShader(directDevice->device, d3dBlobBufferPointer(PixelShaderBlob), d3dBlobBufferSize(PixelShaderBlob),0, 
            &directDevice->psDefault);
   Assert(result == S_OK);
   result = d3d_ps_CreateShader(directDevice->device, d3dBlobBufferPointer(PeelShaderBlob), d3dBlobBufferSize(PeelShaderBlob),0, 
            &directDevice->ps_peeling_shader);
   Assert(result == S_OK);
   result = d3d_ps_CreateShader(directDevice->device, d3dBlobBufferPointer(PixelCompositeShaderBlob), d3dBlobBufferSize(PixelCompositeShaderBlob),0, 
            &directDevice->psCompositeShader);
   Assert(result == S_OK);
   result = d3d_ps_CreateShader(directDevice->device, d3dBlobBufferPointer(psSmoothBlob), d3dBlobBufferSize(psSmoothBlob),0, 
            &directDevice->psSmoothShader);
   Assert(result == S_OK);

   //set default shaders to use on next frame
   d3d_vs_SetShader(directDevice->devicecontext, directDevice->vsDefault, 0, 0);
   d3d_ps_SetShader(directDevice->devicecontext, directDevice->psDefault, 0, 0);

   //VTOut
   //set the input for using on the vertex shader
   D3D11_INPUT_ELEMENT_DESC VertexSpriteData[] = 
   {
       {"POSITION"    , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
       {"TEXCOORD"    , 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
       {"COLOR"       , 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
       {"BLENDINDICES", 0, DXGI_FORMAT_R32_UINT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
   };
   ID3D11InputLayout *inputlayout;
   //Creates input layout from the current shader and sets it for the IA stage.
   //Can it be used for more ??
   //picks the blob of the compiled vertex shader and specifies the input it's going to use
   //for said shader.
   result = d3d11_create_input_layout(directDevice->device,
                                 VertexSpriteData,
                                 4,
								 VertexShaderBlob,
								 &inputlayout);
   Assert(result == S_OK);
   ID3D11DeviceContext_IASetInputLayout(directDevice->devicecontext, inputlayout);
}

static d3d_buffer 
d3d11_create_depth_buffer(d3d_device *directDevice, u32 bufferwidth, u32 bufferheight)
{
    //This shouls ALWAYS have the same size as the back buffer.
    ID3D11Device *device = directDevice->device;
    ID3D11Texture2D          *depthtexture;
    D3D11_TEXTURE2D_DESC      depthtexturedesc = {0};

    ID3D11ShaderResourceView       *depthresourceview;         
    ID3D11DepthStencilView         *depthStencilview; 
    D3D11_DEPTH_STENCIL_VIEW_DESC   depthStencilviewdesc = {0};
    D3D11_SHADER_RESOURCE_VIEW_DESC depthresourceviewdesc = {0};

    depthtexturedesc.Width              = bufferwidth; 
    depthtexturedesc.Height             = bufferheight; 
    depthtexturedesc.MipLevels          = 1;
    depthtexturedesc.ArraySize          = 1;
    depthtexturedesc.Format             = DXGI_FORMAT_R24G8_TYPELESS;
    depthtexturedesc.SampleDesc.Count   = SAMPLING;
    depthtexturedesc.SampleDesc.Quality = 0;
    depthtexturedesc.Usage              = D3D11_USAGE_DEFAULT;
    depthtexturedesc.BindFlags          = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    depthtexturedesc.CPUAccessFlags     = 0; 
    depthtexturedesc.MiscFlags          = 0;
    

   depthStencilviewdesc.Format        = DXGI_FORMAT_D24_UNORM_S8_UINT;
   depthStencilviewdesc.ViewDimension = SAMPLING > 1 ?  D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
   depthStencilviewdesc.Texture2D.MipSlice = 0;

   depthresourceviewdesc.Format                    = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
   depthresourceviewdesc.ViewDimension             = SAMPLING > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
   depthresourceviewdesc.Texture2D.MostDetailedMip = 0;
   depthresourceviewdesc.Texture2D.MipLevels       = 1;
   ID3D11Device_CreateTexture2D(device, &depthtexturedesc, 0, &depthtexture);
   HRESULT result = S_OK;
   result = ID3D11Device_CreateDepthStencilView(device, (ID3D11Resource *)depthtexture, &depthStencilviewdesc, &depthStencilview);
   Assert(result == S_OK);
   result = ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)depthtexture, &depthresourceviewdesc, &depthresourceview);
   Assert(result == S_OK);
   ID3D11DeviceChild_Release(depthtexture);

   d3d_buffer depthbuffer = 
   {depthStencilview, depthresourceview};
   return depthbuffer;
}


RENDER_SWAP_BUFFERS(d3d11_swap_buffers);
RENDER_DRAW_START(d3d11_draw_start);
RENDER_DRAW_END(d3d11_draw_end);
RENDER_PUSH_TEXTURE(d3d11_push_texture);

RENDER_SWAP_BUFFERS(win32_d3d11_swap_buffers)
{
	d3d11_swap_buffers(r, game_renderer);
}

RENDER_DRAW_START(win32_d3d11_draw_start)
{
	d3d11_draw_start(r, game_renderer);
}

RENDER_DRAW_END(win32_d3d11_draw_end)
{ 
	d3d11_draw_end(r, game_renderer);
}
RENDER_PUSH_TEXTURE(win32_d3d11_push_texture)
{
	render_texture result = d3d11_push_texture(
			r,
			pixels,
			w,
			h,
			bpp,
			index);
	return(result);
}

RENDER_ALLOCATE_FRAME_BUFFER(win32_d3d11_allocate_frame_buffer)
{
	d3d_device *direct_device = (d3d_device *)r;
	u32 new_buffer_index = d3d11_create_frame_buffer(
			(d3d_device *)r,
			w,
			h);
	return(new_buffer_index);
}


d3d_device * 
d3d_init(memory_area *area,
        HWND windowhand,
		platform_renderer_init_values initial_values,
		platform_renderer_init_functions *init_only_functions)

{

	u32 texture_array_w = initial_values.texture_array_w;
	u32 texture_array_h = initial_values.texture_array_h;
	u32 texturesCapacity = initial_values.texture_array_capacity;
	u32 maximum_quad_size = initial_values.maximum_quad_size;
	u32 BackBufferWidth = initial_values.back_buffer_width;
	u32 BackBufferHeight  = initial_values.back_buffer_height;

    d3d_device *directDevice  = memory_area_push_struct(area, d3d_device);
	directDevice->header.f_draw_start = win32_d3d11_draw_start;
	directDevice->header.f_draw_end = win32_d3d11_draw_end;
	directDevice->header.f_swap_buffers = win32_d3d11_swap_buffers;
	directDevice->header.f_push_texture = win32_d3d11_push_texture;
	if(init_only_functions)
	{
		init_only_functions->f_allocate_frame_buffer = win32_d3d11_allocate_frame_buffer;
	}

    directDevice->header.type = render_api_d3d11;

    DXGI_SWAP_CHAIN_DESC swapchaindesc = {0}; //Gives instructions to the input pipeline and swaps buffers
    IDXGISwapChain         *swapchain; //The swap chain contains a function to change back buffer size 
    ID3D11Device           *device;
    ID3D11DeviceContext    *devicecontext;
    ID3D11Buffer           *index_buffer;
    ID3D11Buffer           *vertex_buffer;


    HRESULT result = S_OK;

    swapchaindesc.BufferDesc.Width  = BackBufferWidth; //Back buffer width and height
    swapchaindesc.BufferDesc.Height = BackBufferHeight;
    swapchaindesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	/*
	   From Microsoft:
     The DXGI_RATIONAL structure operates under the following rules:
     
     0/0 is legal and will be interpreted as 0/1.
     0/anything is interpreted as zero.
     If you are representing a whole number, the denominator should be 1.
	*/
    swapchaindesc.BufferDesc.RefreshRate.Numerator   = 60; //Detect user monitor hz!
    swapchaindesc.BufferDesc.RefreshRate.Denominator = 1;
    swapchaindesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	//Sequential for ui chain ?
    swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; 
    swapchaindesc.BufferCount = 1;
    swapchaindesc.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
    swapchaindesc.OutputWindow = windowhand;
    swapchaindesc.SampleDesc.Count   = SAMPLING; //sampling
    swapchaindesc.SampleDesc.Quality = 0;
    swapchaindesc.Windowed = TRUE;
    //swapchaindesc.Flags    =  DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapchaindesc.Flags    =  0;
    
	//I'll be back >.>
    D3D_FEATURE_LEVEL flevel;
    //First make sure to actually create the device and swap chain 
    result = D3D11CreateDeviceAndSwapChain( 
                    NULL, 
                    //D3D_DRIVER_TYPE_REFERENCE, //Software
                    D3D_DRIVER_TYPE_HARDWARE,
                    NULL, //for software 
                    D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG, 
                    0, 
                    0, 
                    D3D11_SDK_VERSION, 
                    &swapchaindesc, 
                    &swapchain, 
                    &device, 
                    &flevel, //Not applied
                    &devicecontext);
	//Hardware overlay test
#if 0
	if(0)
	{
	IDXGISwapChain *overlayChain = 0;

         DXGI_SWAP_CHAIN_DESC swapChainDesc = {0}; //Gives instructions to the input pipeline and swaps buffers
         swapChainDesc.BufferDesc.Width  = BackBufferWidth; //Back buffer width and height
         swapChainDesc.BufferDesc.Height = BackBufferHeight;
         swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
         swapChainDesc.BufferDesc.RefreshRate.Numerator   = 60; //Detect user monitor hz!
         swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
         swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

         swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; 
         swapChainDesc.BufferCount = 1;
         swapChainDesc.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
         swapChainDesc.OutputWindow = windowhand;
         swapChainDesc.SampleDesc.Count   = SAMPLING; //sampling
         swapChainDesc.SampleDesc.Quality = 0;
         swapChainDesc.Windowed = TRUE;
         swapChainDesc.Flags    =  DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER;
		//overlay

		 IDXGIFactory *factory;
        CreateDXGIFactory(&IID_IDXGIFactory, &factory);
		IDXGIFactory_CreateSwapChain(factory , device, &swapChainDesc,&overlayChain);
	}
#endif
   directDevice->swapchain        = swapchain; 
   directDevice->device           = device;
   directDevice->devicecontext    = devicecontext;

   Assert(result == S_OK);
    
   D3D11_VIEWPORT vport = {0}; 
   vport.TopLeftX = 0;
   vport.TopLeftY = 0;
   vport.Width  = (real32)BackBufferWidth; //How big the world looks
   vport.Height = (real32)BackBufferHeight;
   vport.MinDepth = 0;
   vport.MaxDepth = 1;

   //Part of the same steps
    
   //BACKBUFFER
   ID3D11RenderTargetView   *backbuffer;
   ID3D11Texture2D          *backbuffertexture; 
   ID3D11ShaderResourceView *backbufferresourceview;
   D3D11_SHADER_RESOURCE_VIEW_DESC backbufferresourcedesc = {0};

   backbufferresourcedesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
   backbufferresourcedesc.ViewDimension = SAMPLING > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
   backbufferresourcedesc.Texture2D.MostDetailedMip = 0;
   backbufferresourcedesc.Texture2D.MipLevels = 1;

   IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D11Texture2D, &backbuffertexture); //Default render target
   ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)backbuffertexture, 0, &backbuffer);
   result = ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)backbuffertexture, &backbufferresourcedesc, &backbufferresourceview);
   Assert(result == S_OK);
   //Create texture2d for texturetarget and then createrendertargetview
   ID3D11DeviceChild_Release(backbuffertexture);


   //VIEWPORT & DEPTHSTENCIL
    D3D11_DEPTH_STENCIL_DESC  depthdesc = {0};
    ID3D11DepthStencilState *depthStencilstate;
    depthdesc.DepthEnable    = 1; //Just in case I want to turn it off.
    depthdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthdesc.DepthFunc      = D3D11_COMPARISON_LESS;

    depthdesc.StencilEnable                = 1;
    depthdesc.StencilReadMask              = 0xff; //0xff is default.
    depthdesc.StencilWriteMask             = 0xff; //0xff is default.
    depthdesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
    depthdesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depthdesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
    depthdesc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;

    depthdesc.BackFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
    depthdesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    depthdesc.BackFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
    depthdesc.BackFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
    //Steps to create a render target
   result = ID3D11Device_CreateDepthStencilState(device, &depthdesc, &depthStencilstate);
   Assert(result == S_OK);
   ID3D11DeviceContext_OMSetDepthStencilState(devicecontext, depthStencilstate ,1);
   ID3D11DeviceContext_RSSetViewports(devicecontext, 1 ,&vport);
   //ID3D11DeviceContext_OMSetRenderTargets(devicecontext ,1, &backbuffer, 0);
   //

   //Load shaders and check if files are found
   //NOTE: I can also use D3DCompile to compile from memory.
   

   //a quad is composed by 4 render_vertex
   u32 QuadSize = sizeof(render_vertex) * 4; 
   u32 qz = maximum_quad_size / QuadSize;
   //6 indices are used in the index buffer for drawing a quad
   u32 index_buffer_indices_count = 6 * qz;
   u32 index_buffer_size = index_buffer_indices_count * sizeof(u16);
   //allocate index buffer
   u16 *index_buffer_memory = 
	   memory_area_push_array(
			   area,
			   u16,
			   index_buffer_indices_count);
   u16 quad_vertex_index = 0;
   //set up the index buffer to specifically draw quads
   for(u32 i = 0; i < index_buffer_indices_count; i += 6)
   {
       index_buffer_memory[i]     = quad_vertex_index; 
       index_buffer_memory[i + 1] = quad_vertex_index + 1; 
       index_buffer_memory[i + 2] = quad_vertex_index + 2; 

       index_buffer_memory[i + 3] = quad_vertex_index; 
       index_buffer_memory[i + 4] = quad_vertex_index + 2; 
       index_buffer_memory[i + 5] = quad_vertex_index + 3; 
       quad_vertex_index += 4;
       if(quad_vertex_index == MAXINDICES)
       {
           quad_vertex_index = 0;
       }
   }
   Assert(quad_vertex_index < U16MAX);

   //create vertex and index buffers
   D3D11_BUFFER_DESC vertex_buffer_desc = {0};
   D3D11_SUBRESOURCE_DATA vertex_buffer_data = {0};
   D3D11_SUBRESOURCE_DATA index_buffer_data  = {0};
   vertex_buffer_data.pSysMem = memory_area_push_size(area, maximum_quad_size); 

   vertex_buffer_desc.ByteWidth = maximum_quad_size; 
   vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
   vertex_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
   vertex_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;

   D3D11_BUFFER_DESC indexdesc = {0};
   index_buffer_data.pSysMem  = index_buffer_memory;

   indexdesc.ByteWidth = index_buffer_size;
   indexdesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
   indexdesc.CPUAccessFlags = 0;// D3D11_CPU_ACCESS_WRITE;
   indexdesc.Usage          = D3D11_USAGE_IMMUTABLE;

   //Creates vertex buffer
   result = ID3D11Device_CreateBuffer(device, &vertex_buffer_desc, &vertex_buffer_data, &vertex_buffer);
   Assert(result == S_OK);
   //create index buffer
   result = ID3D11Device_CreateBuffer(device, &indexdesc, &index_buffer_data, &index_buffer);
   Assert(result == S_OK);
   //Map vertices to buffer.
   UINT vertices_size = sizeof(render_vertex);
   static UINT renderoffsets = 0;
   ID3D11DeviceContext_IASetVertexBuffers(
		   devicecontext,
		   0,
		   1,
		   &vertex_buffer,
		   &vertices_size,
		   &renderoffsets);
   //this doesn't ask for a buffer array like above
   ID3D11DeviceContext_IASetIndexBuffer(devicecontext, index_buffer, DXGI_FORMAT_R16_UINT, 0);
   //The second parameter is 0 because each element uses one unique semantic.
   //The fifth parameter is the offset aligment. I can also use D3D11_APPEND_ALIGNED_ELEMENT 
   //Last value doesn't work with the Vertex directDevice flag.


   //
   //=======render_texture
   //
   ID3D11SamplerState *tsamplerstate;
   D3D11_SAMPLER_DESC tsamplerdesc = {0};

   //TODO: Add sampler state.
#define FILTER_NEAR 1
#define FILTER_LINEAR 1
#if 1
   D3D11_FILTER filter_value = platform_filter_to_d3d11(
		   initial_values.min_filter,
		   initial_values.max_filter,
		   initial_values.mip_filter);
   tsamplerdesc.Filter = filter_value;
#elif 0
   tsamplerdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
   tsamplerdesc.MaxAnisotropy = 16;
#else
   tsamplerdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
   tsamplerdesc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
#endif
//  D3D11_TEXTURE_ADDRESS_WRAP = 1,
//  D3D11_TEXTURE_ADDRESS_MIRROR = 2,
//  D3D11_TEXTURE_ADDRESS_CLAMP = 3,
//  D3D11_TEXTURE_ADDRESS_BORDER = 4,
//  D3D11_TEXTURE_ADDRESS_MIRROR_ONCE = 5

   D3D11_TEXTURE_ADDRESS_MODE uv_adress_mode = platform_uv_adress_to_d3d11(initial_values.uv_adress);
   tsamplerdesc.AddressU = uv_adress_mode;
   tsamplerdesc.AddressV = uv_adress_mode;
   tsamplerdesc.AddressW = uv_adress_mode;
   tsamplerdesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
   tsamplerdesc.MipLODBias = 0;
   tsamplerdesc.MinLOD = 0;
   tsamplerdesc.MaxLOD = D3D11_FLOAT32_MAX;

   result = ID3D11Device_CreateSamplerState(device, &tsamplerdesc, &tsamplerstate);
   Assert(result == S_OK);

   ID3D11Buffer *WVPcb;

   D3D11_BUFFER_DESC WVPbufferdesc = {0};
   WVPbufferdesc.Usage     = D3D11_USAGE_DEFAULT;
   WVPbufferdesc.ByteWidth = sizeof(matrix4x4);
   WVPbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
   result = ID3D11Device_CreateBuffer(device, &WVPbufferdesc, 0, &WVPcb);
   Assert(result == S_OK);
   //matrix4x4 WorldViewProj;

   //Rasterizing 
   ID3D11RasterizerState *rasterizerState;
   D3D11_RASTERIZER_DESC rasterizerDesc = {0};
   rasterizerDesc.FillMode = D3D11_FILL_SOLID; // or D3D11_FILL_WIREFRAME
   rasterizerDesc.CullMode = D3D11_CULL_BACK;
   rasterizerDesc.FrontCounterClockwise= 0;
   //culling
//   rasterizerDesc.CullMode = D3D11_CULL_NONE;
   rasterizerDesc.FrontCounterClockwise = 0;
   rasterizerDesc.DepthBias = 0;
   rasterizerDesc.DepthClipEnable = 1;
   rasterizerDesc.ScissorEnable = 1;
   rasterizerDesc.AntialiasedLineEnable = 1;
   rasterizerDesc.MultisampleEnable = 0;
   ID3D11Device_CreateRasterizerState(device, &rasterizerDesc, &rasterizerState);
   D3D11_RECT ScissorRect = {0, 0, BackBufferWidth, BackBufferHeight};
   ID3D11DeviceContext_RSSetScissorRects(devicecontext, 1, &ScissorRect);

   //Blending
   ID3D11BlendState *blendstate;
   D3D11_BLEND_DESC blenddesc = {0};
   D3D11_RENDER_TARGET_BLEND_DESC targetblenddesc = {0};

   targetblenddesc.BlendEnable            = 1;
   targetblenddesc.SrcBlend               = D3D11_BLEND_SRC_ALPHA;
   targetblenddesc.DestBlend              = D3D11_BLEND_INV_SRC_ALPHA;
   targetblenddesc.BlendOp                = D3D11_BLEND_OP_ADD;

   targetblenddesc.SrcBlendAlpha          = D3D11_BLEND_ONE;
   targetblenddesc.DestBlendAlpha         = D3D11_BLEND_ZERO;
   targetblenddesc.BlendOpAlpha           = D3D11_BLEND_OP_ADD;
   //targetblenddesc.RenderTargetWriteMask  = D3D10_COLOR_WRITE_ENABLE_RED | D3D10_COLOR_WRITE_ENABLE_BLUE | D3D10_COLOR_WRITE_ENABLE_GREEN;
   targetblenddesc.RenderTargetWriteMask  = D3D10_COLOR_WRITE_ENABLE_ALL;

   blenddesc.AlphaToCoverageEnable = 0;
   blenddesc.RenderTarget[0] = targetblenddesc;

   result = ID3D11Device_CreateBlendState(device, &blenddesc, &blendstate);
   Assert(result == S_OK);

   directDevice->vertex_buffer       = vertex_buffer;
   directDevice->index_buffer        = index_buffer;
   directDevice->DefaultDepthState  = depthStencilstate;
   directDevice->camerashaderbuffer = WVPcb;
   directDevice->samplerstate       = tsamplerstate;
   directDevice->blendstate         = blendstate;
   directDevice->texture_array_w  = texture_array_w;
   directDevice->texture_array_h = texture_array_h;
   directDevice->mip_levels = 1;




   u32 depth_peel_layers_count = initial_values.total_depth_peel_layers;

   depth_peel_layers_count = depth_peel_layers_count > 4 ? 4 : depth_peel_layers_count;
   if(!depth_peel_layers_count)
   {
	   depth_peel_layers_count = 4;
   }

   d3d11_create_texture_array(directDevice ,texture_array_w, texture_array_h, directDevice->mip_levels, texturesCapacity);
   directDevice->frame_buffer_max = depth_peel_layers_count + initial_values.display_buffers_count + 1;
   directDevice->frame_buffers = memory_area_push_array(
		   area,
		   d3d_buffer,
		   directDevice->frame_buffer_max);
   directDevice->depth_buffers = memory_area_push_array(
		   area,
		   d3d_buffer,
		   depth_peel_layers_count);
   //set default display buffer
   directDevice->frame_buffers[0].RenderHandle   = backbuffer;
   directDevice->frame_buffers[0].ResourceHandle = backbufferresourceview;
   directDevice->frame_buffer_count++;

   for(u32 f = 0;
		   f < depth_peel_layers_count;
		   f++)
   {
	   d3d11_create_frame_buffer(directDevice, BackBufferWidth, BackBufferHeight);
   }
   for(u32 d = 0;
		   d < depth_peel_layers_count;
		   d++)
   {
	   directDevice->depth_buffers[d] = d3d11_create_depth_buffer(directDevice, BackBufferWidth, BackBufferHeight);
   }
   directDevice->depth_peel_layer_count = depth_peel_layers_count;
   ID3D11DeviceContext_OMSetRenderTargets(devicecontext ,1, &backbuffer, directDevice->depth_buffers[0].RenderHandle);

   ID3D11DeviceContext_IASetPrimitiveTopology(devicecontext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   ID3D11DeviceContext_RSSetState(devicecontext, rasterizerState);

   d3d_InitializeShaderProgram(directDevice);

   return(directDevice); 
}

typedef struct
{
    matrix4x4 WVP;
}CameraShaderBuffer;

typedef struct{
    u32 total_draw_count;
    u32 indexDrawCount;
    u32 vertexDrawStart;

	//total quads in the restorable vertex buffer
    u32 pushedQuadsToBuffer;

	u32 reservedLockedGroupsCount;
	u32 *reservedLockedGroups;

	//for the scissors command
	u16 scissors_on_stack;
	u16 scissor_current;
	u32 scissor_push_count;

	rectangle32s current_draw_clip;

}render_op;

typedef struct{
	u32 use_depth_peeling;
	u32 frame_buffer_count;
}d3d11_initialization_settings;

inline void
d3d_DrawLockedVertices(game_renderer *game_renderer, d3d_device *directDevice, u32 offset)
{
	u32 locked_vertices = game_renderer->quads_locked_from_base;

	//The last parameter is 0 because the base always starts from there.
    ID3D11DeviceContext_DrawIndexed(directDevice->devicecontext, locked_vertices * 6, 0, 0);
}

static void
d3d11_push_quad(game_renderer *game_renderer,
            render_vertex *vertices,
            u32 vertexArrayOffset)
{
   //Offset the locked vertices from base
   u32 quads_locked_from_base = game_renderer->quads_locked_from_base;

   u32 drawOffset = (quads_locked_from_base + vertexArrayOffset) * 4; 

   render_vertex *vertex_buffer = game_renderer->current_vertex_buffer;
   render_vertex *vertex_buffer_at = (vertex_buffer + drawOffset);
   //4 because quad.
   for(int v = 0; v < 4; v++)
   {
     vertex_buffer_at->location = vertices[v].location;
     vertex_buffer_at->uv       = vertices[v].uv;
     vertex_buffer_at->color    = vertices[v].color;
     vertex_buffer_at->texture  = vertices[v].texture;

     vertex_buffer_at++;
   }
   Assert(vertex_buffer_at <
		  game_renderer->current_vertex_buffer + (game_renderer->max_quad_draws * 4));

   game_renderer->draw_count += 4;
   //CopyTo(DrawData, vertices, QUADSIZE);
}


//see notes
static void
d3d_draw_indexed_offset(d3d_device *directDevice,
                        u32 vertexArrayOffset,
						u32 total_draw_count)
{
    u32 current_draw_count = total_draw_count;
    u32 vertex_buffer_offset = vertexArrayOffset;

	//the number of times we advance through the
	//vertex buffer when reading from the index buffer
	//since the index buffer can't hold values more than 65535
	//every time we read from the index buffer, the maximum
	//number of readed indices is capped to that value, and
	//every time I need to keep drawing, vertex_buffer_offset
	//advances through the vertex buffer and starts reading
	//from another index.
    i32 total_vertex_buffer_offsets = current_draw_count / MAXIMUM_QUAD_DRAWS_IN_INDEX_BUFFER;
    u32 draw_count = 0;
    while(total_vertex_buffer_offsets >= 0)
    {
		//cap draw count if we are going to keep offseting
		//through te vertex buffer
       draw_count = total_vertex_buffer_offsets ?
		   MAXIMUM_QUAD_DRAWS_IN_INDEX_BUFFER : current_draw_count; 
       current_draw_count -= MAXIMUM_QUAD_DRAWS_IN_INDEX_BUFFER;

	   //if capped, read 65535 indices from the index buffer,
	   //starting from the specified offset in the vertex buffer.
	   //else the remaining amount of draw counts
       ID3D11DeviceContext_DrawIndexed(directDevice->devicecontext,
                                       draw_count * 6,
                                       0,
                                       vertex_buffer_offset * 4);
	   //advance the amount of drawn quads in the vertex buffer
       vertex_buffer_offset += draw_count;
       total_vertex_buffer_offsets--;
    }
}

//draw on this batch without advancing nor reseting it
static void
d3d_re_draw_indexed(d3d_device *directDevice, render_op *batch)
{
    u32 total_draw_count = batch->indexDrawCount;
    u32 vertexOffset = batch->vertexDrawStart;

    d3d_draw_indexed_offset(directDevice,
                            vertexOffset,
						    total_draw_count);

}

inline void
d3d11_draw_indexed(d3d_device *directDevice, render_op *batch)
{
   d3d_re_draw_indexed(directDevice, batch);
   //advance the batch to the next index group
   batch->vertexDrawStart += batch->indexDrawCount;
   //reset batch count
   batch->indexDrawCount = 0;
}

static void
d3d_draw_locked_groups(d3d_device *directDevice, game_renderer *game_renderer, render_op *batch)
{

	for(u32 l = 0;
			l < batch->reservedLockedGroupsCount;
			l++)
	{
	    u32 lockGroupIndex = batch->reservedLockedGroups[l];


	    render_locked_vertices_group lockedGroup = game_renderer->locked_vertices_groups[lockGroupIndex];
	    d3d_draw_indexed_offset(directDevice, lockedGroup.offset, lockedGroup.count);
	}
}

inline void
d3d_SetBlendState(d3d_device *directDevice, u32 value)
{
    ID3D11BlendState *blendstate = value ? directDevice->blendstate : 0;
   ID3D11DeviceContext_OMSetBlendState(directDevice->devicecontext, blendstate, 0, 0xffffffff);
}

inline void
d3d_set_render_target(
		d3d_device *directDevice,
		ID3D11DepthStencilView *depthBuffer,
		u32 framebufferindex)
{
	ID3D11RenderTargetView *renderTarget = 
		directDevice->frame_buffers[framebufferindex].RenderHandle;
   ID3D11DeviceContext_OMSetRenderTargets(directDevice->devicecontext,
										  1, 
										  &renderTarget,
										  depthBuffer);
   directDevice->last_frame_buffer_index = directDevice->current_frame_buffer_index;
   directDevice->current_frame_buffer_index = framebufferindex;
}

inline void
d3d_SetDepthTest(d3d_device *directDevice, u32 depthenable)
{
  ID3D11DepthStencilState *depthStencilstate = directDevice->DefaultDepthState;
  D3D11_DEPTH_STENCIL_DESC depthStencildesc = {0};
  depthStencilstate->lpVtbl->GetDesc(depthStencilstate, &depthStencildesc);
  depthStencildesc.DepthEnable   = depthenable;
  depthStencildesc.StencilEnable = depthenable;

  ID3D11Device_CreateDepthStencilState(directDevice->device, &depthStencildesc, &depthStencilstate);
  ID3D11DeviceContext_OMSetDepthStencilState(directDevice->devicecontext, depthStencilstate ,1);
}
inline void
d3dSwitchDepthWrite(d3d_device *directDevice, u32 writemask, D3D11_COMPARISON_FUNC comparison )
{
  ID3D11DepthStencilState *depthStencilstate = directDevice->DefaultDepthState;
  D3D11_DEPTH_STENCIL_DESC depthStencildesc = {0};
  depthStencilstate->lpVtbl->GetDesc(depthStencilstate, &depthStencildesc);

  depthStencildesc.DepthWriteMask = writemask;
  depthStencildesc.DepthFunc       = comparison;
  ID3D11Device_CreateDepthStencilState(directDevice->device, &depthStencildesc, &depthStencilstate);

  ID3D11DeviceContext_OMSetDepthStencilState(directDevice->devicecontext, depthStencilstate ,1);
}
inline void
d3d_clear(d3d_device *directDevice, real32 clear_color[4])
{
  ID3D11RenderTargetView *rendertarget = 
	  directDevice->frame_buffers[directDevice->current_frame_buffer_index].RenderHandle;
  ID3D11DeviceContext_ClearRenderTargetView(
		  directDevice->devicecontext, rendertarget, clear_color);
}

//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
inline void
d3d_ps_UnbindShaderResource(d3d_device *directDevice, u32 slotStart, u32 slotCount)
{
	//start from slot start, and set resources from count
	//4 is just an arbitrary number.
   ID3D11ShaderResourceView *unbind[4] = {0};

   ID3D11DeviceContext_PSSetShaderResources(
		   directDevice->devicecontext,
		   slotStart,
		   slotCount,
		   unbind);
}

inline void
d3d_SetClip(d3d_device *directDevice, i32 x0, i32 y0, i32 x1, i32 y1)
{
   D3D11_RECT ScissorRect = {x0, y0, x1, y1};

   ID3D11DeviceContext_RSSetScissorRects(directDevice->devicecontext, 1, &ScissorRect);
}

inline void
d3d_SetClipf(d3d_device *directDevice, real32 x0, real32 y0, real32 x1, real32 y1)
{
   d3d_SetClip(directDevice ,(LONG)x0, (LONG)y0, (LONG)x1, (LONG)y1);
}


inline void
d3d_SetViewport(d3d_device *directDevice, real32 x0, real32 y0, real32 x1, real32 y1)
{
    D3D11_VIEWPORT vport = {0}; 
    
    vport.TopLeftX = x0;
    vport.TopLeftY = y0;
    vport.Width    = x1 - x0; //How big the world looks
    vport.Height   = y1 - y0;
    vport.MinDepth = 0;
    vport.MaxDepth = 1;

   ID3D11DeviceContext_RSSetViewports(directDevice->devicecontext, 1 ,&vport);
   //ID3D11DeviceContext_RSSetViewports(directDevice->devicecontext, 0 ,&vport);
}

inline void
d3d11_set_screen_viewport_and_clip(d3d_device *directDevice, real32 x0, real32 y0, real32 x1, real32 y1)
{
    D3D11_VIEWPORT vport = {0}; 
    
    vport.TopLeftX = x0;
    vport.TopLeftY = y0;
    vport.Width    = x1 - x0; //How big the world looks
    vport.Height   = y1 - y0;
    vport.MinDepth = 0;
    vport.MaxDepth = 1;
   D3D11_RECT ScissorRect = {(LONG)x0, (LONG)y0, (LONG)x1, (LONG)y1};

   ID3D11DeviceContext_RSSetViewports(directDevice->devicecontext, 1 ,&vport);
   ID3D11DeviceContext_RSSetScissorRects(directDevice->devicecontext, 1, &ScissorRect);
}


static void
d3d11_push_texture_image(d3d_device *directDevice,
                   image_data *image, 
                   i32 index)
{
   ID3D11DeviceContext *devicecontext = directDevice->devicecontext;

   ID3D11Texture2D *texturearray;
   ID3D11View_GetResource(directDevice->textureArrayView, &(ID3D11Resource *)texturearray);

   u32 Pitch = directDevice->texture_array_w * 4;
   u32 SizePixels = directDevice->texture_array_w * directDevice->texture_array_h;
   u32 MipLevels  = directDevice->mip_levels;

   UINT CALCSR = index * directDevice->mip_levels; //Instead of D3D11CalcSubresource
   
   u32 mip = 0;
   while(mip++ < directDevice->mip_levels)
   {
       Pitch      = image->width * image->bpp;
       SizePixels = image->width * image->height;

       ID3D11DeviceContext_UpdateSubresource(devicecontext, (ID3D11Resource *)texturearray,
                                             CALCSR,
                                             0,
                                             image->pixels,
                                             Pitch,
                                             SizePixels);    
       if(directDevice->mip_levels > 1)
       {
         down_sample_image_linear_x2(image);
       }
       CALCSR += 1;
   }
   ID3D11DeviceChild_Release(texturearray);
}

RENDER_PUSH_TEXTURE(d3d11_push_texture)
{
	d3d_device *directDevice = (d3d_device *)r;
    render_texture result = {0};
    result.width = w;
    result.height = h;
    result.index = index;
    image_data TEMPimageData = {0};
    TEMPimageData.pixels = pixels;
    TEMPimageData.width  = w;
    TEMPimageData.height = h;
    TEMPimageData.bpp    = bpp;
    
    d3d11_push_texture_image(directDevice, &TEMPimageData, index);

    return(result);
}

static render_texture 
d3dPushWhiteImage(memory_area *memory,
                  d3d_device *directDevice,
                  i32 offset)
{

   temporary_area temparea = temporary_area_begin(memory);

   u32 textureArrayW = directDevice->texture_array_w;
   u32 textureArrayH = directDevice->texture_array_h;
   u32 imageSize = textureArrayW * textureArrayH * 4;
   u32 *whitemem = (u32 *)memory_area_push_size(memory, imageSize);
   for(u32 i = 0; i < imageSize; i++)
   {
       whitemem[i] = 0xffffffff;
   }

   render_texture result = d3d11_push_texture(
		   &directDevice->header,
		   (u8 *)whitemem,
		   textureArrayW,
		   textureArrayH,
		   4,
		   offset);
   temporary_area_end(&temparea);
   return(result);
}



static void
d3d_read_commands(d3d_device *directDevice,
                game_renderer *game_renderer,
                render_commands *rendercommands,
                render_op *batch,
				u32 processingPeeling)
{

   u32 errorTestIndex = 0;
   u8 *command = rendercommands->commands_base;
   rectangle32s currentClip = {0};
   while(command != rendercommands->commands_offset)
   {
       errorTestIndex++;
       render_command_type *renderheader = (render_command_type *)command;
       //Get the data here.
       u32 offset = 0;
       switch(*renderheader)
       {
           case render_command_type_clear:
               {
                   render_command_clear *data = (render_command_clear *)command;
                   offset = sizeof(render_command_clear);
                   d3d_clear(directDevice, game_renderer->clear_color);
                   //Clear screen
               }break;
           case render_command_type_drawquad:
               {
                   render_command_drawquad *data = (render_command_drawquad *)command;
                   offset = sizeof(render_command_drawquad);
                   //DrawQuad
                   d3d11_push_quad(game_renderer, data->vertices, batch->pushedQuadsToBuffer); 
				   batch->pushedQuadsToBuffer++;
                   batch->indexDrawCount++;

               }break;

           case render_command_type_PushClip:
               {
                   render_command_SetClip *data = (render_command_SetClip *)command;
                   offset = sizeof(render_command_SetClip);

				   //Get the first scissor from the stack
				   render_scissor *scissorPushed = game_renderer->scissor_stack + batch->scissor_push_count;
				   i32 cX0 = scissorPushed->clip.x0;
				   i32 cY0 = scissorPushed->clip.y0;
				   i32 cX1 = scissorPushed->clip.x1;
				   i32 cY1 = scissorPushed->clip.y1;

				   batch->scissor_current = batch->scissor_push_count;
				   batch->scissors_on_stack++;
				   batch->scissor_push_count++;
				   
				   //;Cleanup
					u32 tempScissorStackCount = game_renderer->scissor_total_count; 
				    Assert(batch->scissors_on_stack < tempScissorStackCount);
				   //
				   //Draw current quads before switching clip.
				   //
                   d3d11_draw_indexed(directDevice, batch);
				   d3d_SetClip(directDevice, cX0, cY0, cX1, cY1);

				   currentClip.x0 = cX0;
				   currentClip.y0 = cY0;
				   currentClip.x1 = cX1;
				   currentClip.y1 = cY1;
                   //DrawQuad

               }break;
		   case render_command_type_PopClip:
			   {
                    offset = sizeof(render_command_PopClip);
					batch->scissors_on_stack--;

					u32 tempScissorStackCount = game_renderer->scissor_total_count; 
				    Assert(batch->scissors_on_stack < tempScissorStackCount);
				    //
				    //Restore clip
				    //Pop stack?
					i32 cX0 = 0; 
					i32 cY0 = 0; 
					i32 cX1 = 0; 
					i32 cY1 = 0; 

				    render_scissor *scissor_stack = game_renderer->scissor_stack;
					if(batch->scissors_on_stack > 0)
					{
					   batch->scissor_current = scissor_stack[batch->scissor_current].previous;
					   u32 sI = batch->scissor_current;
					   cX0 = scissor_stack[sI].clip.x;
					   cY0 = scissor_stack[sI].clip.y;
					   //These where already clipadded before.
					   cX1 = scissor_stack[sI].clip.w;
					   cY1 = scissor_stack[sI].clip.h;


					}
					else
					{
				       cX0 = batch->current_draw_clip.x; 
   				       cY0 = batch->current_draw_clip.y;
   				       cX1 = batch->current_draw_clip.w;
   				       cY1 = batch->current_draw_clip.h;
					}
                    d3d11_draw_indexed(directDevice, batch);
				    d3d_SetClip(directDevice, cX0, cY0, cX1, cY1);

					currentClip.x0 = cX0;
					currentClip.y0 = cY0;
					currentClip.x1 = cX1;
					currentClip.y1 = cY1;


			   }break;
		   case render_command_type_draw_locked_vertices:
			   {
				   offset = sizeof(render_command_draw_locked_vertices_data);
                   render_command_draw_locked_vertices_data *data = (render_command_draw_locked_vertices_data *)command;

				   u32 lockGroupIndex = data->groupIndex;

				   Assert(lockGroupIndex < game_renderer->locked_vertices_group_count);

				   render_locked_vertices_group *lockedGroup = game_renderer->locked_vertices_groups + data->groupIndex;


				   if(!processingPeeling)
				   {
				       //Draw previous vertices (+1 extra draw call)
                       d3d11_draw_indexed(directDevice, batch);

				       d3d_draw_indexed_offset(directDevice, lockedGroup->offset, lockedGroup->count);
				   }
				   else
				   {
					   batch->reservedLockedGroups[batch->reservedLockedGroupsCount++] = lockGroupIndex;
				   }
			   }break;
           default:
               {
                   Assert(0);
               }
       }
       command += offset;
       offset = 0;
   }
   Assert(errorTestIndex == rendercommands->command_count);
    //TODO:Get data and header in order to execute the given command.
}

inline void
d3d_UpdateShaderCamera(d3d_device *directDevice, matrix4x4 wvp)
{
     CameraShaderBuffer csb = {0}; 
     csb.WVP = wvp;
     ID3D11Buffer *CameraBuffer = directDevice->camerashaderbuffer; //update this buffer and set it
     ID3D11DeviceContext_UpdateSubresource(directDevice->devicecontext, (ID3D11Resource *)CameraBuffer, 0, 0, &csb, 0,0); 
     ID3D11DeviceContext_VSSetConstantBuffers(directDevice->devicecontext, 0, 1, &CameraBuffer);
}

typedef struct{
	render_commands *commandsAfterReserved;

	u16 processingPeeling;


	u32 reserved_render_commands_at;
	u16 reserved_render_commands_count;
	u16 reserved_render_commands_max;
	render_commands *reserved_render_commands;
}render_commands_op;

RENDER_SWAP_BUFFERS(d3d11_swap_buffers)
{
	d3d_device *directDevice = (d3d_device *)r;
	//I can use different swap chains for rendering other stuff like ui.
	//In this case this swap chain is for the game.
    IDXGISwapChain_Present(directDevice->swapchain, 0, 0);
}

RENDER_DRAW_START(d3d11_draw_start)
{
	d3d_device *directDevice = (d3d_device *)r;
   D3D11_MAPPED_SUBRESOURCE current_vertex_buffer; 

   ID3D11DeviceContext_Map(directDevice->devicecontext,
		                   (ID3D11Resource *)directDevice->vertex_buffer,
						   0,
						   D3D11_MAP_WRITE_DISCARD,
						   0,
						   &current_vertex_buffer);
   game_renderer->current_vertex_buffer = current_vertex_buffer.pData;
   ID3D11DeviceContext_Unmap(directDevice->devicecontext,
		                     (ID3D11Resource *)directDevice->vertex_buffer,
							 0);
}

RENDER_DRAW_END(d3d11_draw_end)
{ 
	d3d_device *directDevice = (d3d_device *)r;
   //D3D11_MAPPED_SUBRESOURCE current_vertex_buffer; 
   //ID3D11DeviceContext_Map(directDevice->devicecontext, (ID3D11Resource *)directDevice->vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &current_vertex_buffer);
   //game_renderer->current_vertex_buffer = current_vertex_buffer.pData;
   //ID3D11DeviceContext_Unmap(directDevice->devicecontext, (ID3D11Resource *)directDevice->vertex_buffer, 0);

   //Clear black bars
   real32 clear_color[] = {0,0 ,0, 0};
   d3d_clear(directDevice, clear_color);
#if 0
   real32 clipX0 = 0; 
   real32 clipY0 = 0;
   real32 clipX1 = (real32)game_renderer->back_buffer_width;
   real32 clipY1 = (real32)game_renderer->back_buffer_height; 
#else
   real32 clipX0 = (real32)game_renderer->game_draw_clip.x; 
   real32 clipY0 = (real32)game_renderer->game_draw_clip.y;
   real32 clipX1 = (real32)game_renderer->game_draw_clip.w;
   real32 clipY1 = (real32)game_renderer->game_draw_clip.h;
#endif
   d3d11_set_screen_viewport_and_clip(directDevice, clipX0, clipY0, clipX1, clipY1); 


   d3d_ps_SetShader(directDevice->devicecontext ,directDevice->psDefault, 0, 0);
   ID3D11DeviceContext_PSSetSamplers(directDevice->devicecontext, 0, 1, &directDevice->samplerstate);
   ID3D11DeviceContext *devicecontext = directDevice->devicecontext;
   //Maximum resources are 128. i.e D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT. 
   ID3D11DeviceContext_PSSetShaderResources(directDevice->devicecontext,
                                            0, 1, &directDevice->textureArrayView);
   //This doesn't matter unless I specify to use the factor on source or destination.
  // ID3D11DeviceContext_OMSetBlendState(directDevice->devicecontext,directDevice->blendstate, blendfactor, 0xffffffff);

   d3d_ps_SetShader(directDevice->devicecontext ,directDevice->psDefault, 0, 0);
//   d3d_vs_SetShader(directDevice->devicecontext ,directDevice->vsDefault, 0, 0);

   for(i32 depthPeelI = 0; depthPeelI < 4; depthPeelI++)
   {

		ID3D11DeviceContext_ClearDepthStencilView(directDevice->devicecontext,
												   directDevice->depth_buffers[depthPeelI].RenderHandle,
												   D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
   }


   u32 peel_count = 0;
   u32 total_peel_count = directDevice->depth_peel_layer_count;
   render_op batch = {0};
   batch.total_draw_count  = game_renderer->draw_count;
   batch.vertexDrawStart = game_renderer->quads_locked_from_base;
   batch.reservedLockedGroups = render_push_to_command_buffer(
		   game_renderer, sizeof(u32) * 400);

   batch.current_draw_clip = game_renderer->game_draw_clip;

   //Get the render commands from the same buffer
   render_commands *next_render_commands = 
	   (render_commands *)game_renderer->render_commands_buffer_base;
   //commands_op are used to keep them in order
   render_commands_op commands_op = {0};
   commands_op.reserved_render_commands_max = 40;
   u32 totalReservedCommands = commands_op.reserved_render_commands_max * sizeof(render_commands);
   commands_op.reserved_render_commands = render_push_to_command_buffer(
		   game_renderer, totalReservedCommands);

   u32 depth_peel_commands_count = 0;

   u32 commandIndex = 0;
   u32 processing_render_commands = game_renderer->begin_count > 0;

   while(processing_render_commands)
   {
	     //run the reserved commands after the depth peeling was processed
	   //this is true after the depth peeling was already processed
	   //and there are reserved commands
	   if(commands_op.reserved_render_commands_count && !commands_op.processingPeeling) 
	   {
		   Assert(commands_op.processingPeeling == 0);
		   //If this is the first pass to the reserved commands
		   if(!commands_op.commandsAfterReserved)
		   {
			   commands_op.commandsAfterReserved = next_render_commands;
		   }

		   if(commands_op.reserved_render_commands_at == commands_op.reserved_render_commands_count)
		   {
			   commands_op.reserved_render_commands_count = 0;
			   next_render_commands = commands_op.commandsAfterReserved;
		   }
		   else
		   {
			   next_render_commands = 
				   commands_op.reserved_render_commands + commands_op.reserved_render_commands_at++;
		   }

	   }

       render_commands *current_processing_commands = next_render_commands;
	   //if there are more commands after this, then it is at the end of the current
	   next_render_commands = (render_commands *)current_processing_commands->commands_offset;

	   //If it doesn't have the peeling option but a peeling group is in process,
	   //reserve these commands for when the depth peeling got processed
	   while(!(current_processing_commands->render_flags & render_flags_DepthPeel) &&
			   commands_op.processingPeeling)
	   {
	       // ;Assert for now
	       Assert(commands_op.reserved_render_commands_count < commands_op.reserved_render_commands_max);
	       //Skip these commands for the moment and reserve them for post-peeling
	       commands_op.reserved_render_commands[commands_op.reserved_render_commands_count++] = *current_processing_commands;
	       processing_render_commands = 1;

	       //Advance commands and make sure the next ones are part of the peeling group
	       current_processing_commands = next_render_commands;
	       next_render_commands = (render_commands *)current_processing_commands->commands_offset;
	   }


       u16 Blending          = current_processing_commands->render_flags & render_flags_Blending;
       u16 DepthTest         = current_processing_commands->render_flags & render_flags_DepthTest;
       u16 DepthPeel         = current_processing_commands->render_flags & render_flags_DepthPeel;

	   depth_peel_commands_count += (DepthPeel > 0);
	   commands_op.processingPeeling = (depth_peel_commands_count > 0);

	   commandIndex++;
	   processing_render_commands = (commandIndex < game_renderer->begin_count) || commands_op.reserved_render_commands_count > 0;
	   //Choose the first render target
       u16 rendertargetindex = DepthPeel > 0; 

	   if(current_processing_commands->set_viewport_and_clip)
	   {
	       batch.current_draw_clip = current_processing_commands->viewport_and_clip;
           d3d11_set_screen_viewport_and_clip(
				   directDevice,
				   (f32)current_processing_commands->viewport_and_clip.x0,
				   (f32)current_processing_commands->viewport_and_clip.y0,
				   (f32)current_processing_commands->viewport_and_clip.x1,
				   (f32)current_processing_commands->viewport_and_clip.y1); 
	   }

       d3d_set_render_target(
			   directDevice,
			   directDevice->depth_buffers[0].RenderHandle,
			   rendertargetindex);
       d3d_SetBlendState(directDevice, Blending);
       d3d_SetDepthTest(directDevice, DepthTest);

	   matrix4x4 camera_projection = matrix4x4_Identity();
	   if(current_processing_commands->camera_type == render_camera_perspective)
	   {
		   camera_projection = game_renderer->projection;
	   }
	   else if(current_processing_commands->camera_type == render_camera_2d)
	   {
		   camera_projection = game_renderer->projection_2d;
	   }
	   else if(current_processing_commands->camera_type == render_camera_scale_to_display)
	   {
		   camera_projection = render_set_scaled_to_display(game_renderer);
	   }
       
       d3d_UpdateShaderCamera(directDevice, camera_projection);

       d3d_read_commands(directDevice, game_renderer, current_processing_commands, &batch, commands_op.processingPeeling);

	   //Commands were readed, so advance
       if(DepthPeel && depth_peel_commands_count == game_renderer->depth_peel_calls)
	   {
		   //End
		   depth_peel_commands_count = 0;
		   //All of the depth peeling commands are going to be drawn, so this boolean
		   //gets restarted.
		   commands_op.processingPeeling = 0;
		   do
		   {
			   if(peel_count == total_peel_count - 1)
			   {
				   d3d_clear(directDevice, game_renderer->clear_color);

				   //Draw normally and flush the draw call count

				   // ;THIS DOES NOT RESPECT THE ORDER OR DRAWS!!!!!!!!!!!!!!!
				   d3d_draw_locked_groups(directDevice, game_renderer, &batch);
				   d3d11_draw_indexed(directDevice, &batch);


				   d3d_ps_UnbindShaderResource(directDevice, 1, 1);
				   d3d_set_render_target(directDevice,
						   directDevice->depth_buffers[0].RenderHandle,
						   1);
				   //Come back to the normal 3d pixel shader
				   ID3D11DeviceContext_PSSetShader(
						   directDevice->devicecontext,
						   directDevice->psDefault,
						   0,
						   0);
				   break;
			   }
			   //Starts
			   else
			   {
				   if(peel_count)
				   {
					   d3d_clear(directDevice, game_renderer->clear_color);
				   }
				   else
				   {
					   //set the peel shader used for depth peeling
					   ID3D11DeviceContext_PSSetShader(
							   directDevice->devicecontext,
							   directDevice->ps_peeling_shader,
							   0,
							   0);
				   }

				   // ;THIS DOES NOT RESPECT THE ORDER OR DRAWS!!!!!!!!!!!!!!!
				   d3d_draw_locked_groups(directDevice, game_renderer, &batch);
				   d3d_re_draw_indexed(directDevice, &batch);
				   //draw locked vertices

				   d3d_ps_UnbindShaderResource(directDevice, 1, 1);

				   //Compares previous depth buffer with the next.
				   //This starts with the render target 2 because the first one was already used.
				   //Different render targets are needed in order to display the blended colors correctly.
				   d3d_set_render_target(directDevice,
						   directDevice->depth_buffers[peel_count + 1].RenderHandle,
						   peel_count + 2);
				   //Send previous depth buffer 
				   ID3D11ShaderResourceView *svv = 
					   directDevice->depth_buffers[peel_count].ResourceHandle;
				   ID3D11DeviceContext_PSSetShaderResources(
						   directDevice->devicecontext,
						   1,
						   1,
						   &svv);


			   }
		   }while(peel_count++ < total_peel_count);

		   //NOTE(Agu) I might want to set the camera to the default value after this pass... in case
		   d3d_SetViewport(directDevice, 0, 0, (real32)game_renderer->back_buffer_width, (real32)game_renderer->back_buffer_height); 

		   d3d_SetBlendState(directDevice, 0);
		   d3d_set_render_target(directDevice, 0, 0);
		   d3d_SetDepthTest(directDevice, 0);

		   ID3D11DeviceContext_ClearDepthStencilView(directDevice->devicecontext,
				   directDevice->depth_buffers[0].RenderHandle, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		   d3d_ps_SetShader(directDevice->devicecontext ,directDevice->psCompositeShader, 0, 0);
		   d3d_UpdateShaderCamera(directDevice, matrix4x4_Identity());

		   //Send buffers as resources to merge them together
		   ID3D11ShaderResourceView *svv[] = { directDevice->frame_buffers[1].ResourceHandle,
			   directDevice->frame_buffers[2].ResourceHandle,
			   directDevice->frame_buffers[3].ResourceHandle,
			   directDevice->frame_buffers[4].ResourceHandle};

		   ID3D11DeviceContext_PSSetShaderResources(directDevice->devicecontext, 1, 4, &svv[0]);

		   //Note(Agu): Location and UVs
		   render_vertex renderTargetVertex[] = 
		   {
			   {-1, -1, 1,  0, 1, 0},
			   {-1,  1, 1,  0, 0, 0},
			   { 1,  1, 1,  1, 0, 0},
			   { 1, -1, 1,  1, 1, 0}
		   };

		   u32 last_quad_offset = (
				   game_renderer->max_quad_draws - 
				   game_renderer->quads_locked_from_base - 4);
		   d3d11_push_quad(game_renderer,
				   renderTargetVertex,
				   last_quad_offset);
		   /*The locked amount gets added on d3d11_push_quad,
			 so re-add it here in order to get the correct base*/
		   last_quad_offset += game_renderer->quads_locked_from_base;
		   ID3D11DeviceContext_DrawIndexed(directDevice->devicecontext,
				   6,
				   0, 
				   last_quad_offset * 4);

		   d3d_ps_UnbindShaderResource(directDevice, 1, 4);

		   d3d_SetDepthTest(directDevice, 1);
		   d3d_ps_SetShader(directDevice->devicecontext ,directDevice->psDefault, 0, 0);
		   d3d_SetViewport(directDevice, clipX0, clipY0, clipX1, clipY1); 

	   }
       else if(!DepthPeel)
       {
           d3d11_draw_indexed(directDevice, &batch);
       }

	   if(current_processing_commands->restore_viewport_and_clip)
	   {
		   d3d11_set_screen_viewport_and_clip(directDevice, clipX0, clipY0, clipX1, clipY1); 
	   }
       //d3d_DrawLockedVertices(game_renderer, directDevice, 0);
   }
   //draw end




#if 0
   d3d_set_render_target(directDevice, 0, 0);
   real32 clear_color[] = {0 ,0 ,0, 0};
   d3d_clear(directDevice, clear_color);
   d3d_ps_SetShader(directDevice->devicecontext ,directDevice->ppresentshad, 0, 0);
   d3d_vs_SetShader(directDevice->devicecontext ,directDevice->vpresentshad, 0, 0);

   ID3D11ShaderResourceView *svv = directDevice->frame_buffers[4].ResourceHandle;
   ID3D11DeviceContext_PSSetShaderResources(directDevice->devicecontext, 1, 1, &svv);
   render_vertex TargetVertex[] = 
   {
      Vertex(-1, -1, 1,  0, 1, 0),
      Vertex(-1,  1, 1,  0, 0, 0),
      Vertex( 1,  1, 1,  1, 0, 0),
      Vertex( 1, -1, 1,  1, 1, 0)
   };
   d3dDrawQuad(game_renderer, TargetVertex, drawoffset);
   ID3D11DeviceContext_DrawIndexed(directDevice->devicecontext, 6, drawoffset * 6, 0); 
   d3d_ps_UnbindShaderResource(directDevice, 1, 1);
#elif 0


   d3d_SetBlendState(directDevice, 0);
   d3d_set_render_target(directDevice, 0, 0);
   d3d_SetDepthTest(directDevice, 0);
   real32 clear_color[] = {0 ,0 ,0, 0};
   d3d_clear(directDevice, clear_color);
  ID3D11DeviceContext_ClearDepthStencilView(directDevice->devicecontext,
                                            directDevice->depth_buffers[0].RenderHandle, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

   d3d_ps_SetShader(directDevice->devicecontext ,directDevice->psCompositeShader, 0, 0);
   d3d_vs_SetShader(directDevice->devicecontext ,directDevice->vpresentshad, 0, 0);

   ID3D11ShaderResourceView *svv[] = { directDevice->frame_buffers[1].ResourceHandle,
                                       directDevice->frame_buffers[2].ResourceHandle,
                                       directDevice->frame_buffers[3].ResourceHandle,
                                       directDevice->frame_buffers[4].ResourceHandle};

   ID3D11DeviceContext_PSSetShaderResources(directDevice->devicecontext, 0, 4, &svv[0]);
   render_vertex TargetVertex[] = 
   {
      Vertex(-1, -1, 1,  0, 1, 0),
      Vertex(-1,  1, 1,  0, 0, 0),
      Vertex( 1,  1, 1,  1, 0, 0),
      Vertex( 1, -1, 1,  1, 1, 0)
   };
   d3dDrawQuad(game_renderer, TargetVertex, drawoffset);
   ID3D11DeviceContext_DrawIndexed(directDevice->devicecontext, 6, drawoffset * 6, 0); 

   d3d_ps_UnbindShaderResource(directDevice, 0, 4);
   d3d_SetDepthTest(directDevice, 1);

#endif 
}


platform_renderer *
win32_load_renderer(memory_area *area,
        HWND windowhand,
		platform_renderer_init_values initial_values,
		platform_renderer_init_functions *init_functions)
{
	return((platform_renderer *)d3d_init(
				area,
				windowhand,
				initial_values,
				init_functions));
}

int _DllMainCRTStartup()
{
	return(1);
}

