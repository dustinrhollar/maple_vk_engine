/* date = August 15th 2020 4:28 pm */

#ifndef GRAPHICS_MAPLE_GRAPHICS_H
#define GRAPHICS_MAPLE_GRAPHICS_H

/*
Example Psuedocode for rendering a frame

CommandList = CreateCommandList

BeginCommandList(CommandList) <- TODO I DONT THINK THIS IS I<PLEMENTED

cmd_set_camera(CommandList, Projection, View)

for every object
            *   cmd_bind_pipeline(CommandList, Object->Pipeline)
   *   cmd_set_object_data(CommandList, Position, Scale, Rotation)
*   
*   Set any other descriptors
*   
*   cmd_draw

ExecuteCommandList(CommandList) <- TODO NEEDS TO BE HIDDEN BEHIND END_FRAME

*/

// TODO(Dustin): 
// Creation Functions
// - Descriptor Pool
// ---- Create/Destroy
// - Push Constants
// ---- Create/Destroy
// Command Functions
// - Bind Descriptor Set
// - Update Uniform Buffer
// - Update Dynamic Uniform Buffer
// - Update Push Constants

#ifdef __cplusplus
extern "C" 
{   // only need to export C interface if
    // used by C++ source code
#endif
    
#if defined(GRAPHICS_DLL_EXPORT)
    
#define GRAPHICS_API __declspec(dllexport)
#define GRAPHICS_CALL __cdecl
    
#else
    
#define GRAPHICS_API __declspec(dllimport)
#define GRAPHICS_CALL
    
#endif // GRAPHICS_DLL_EXPORT 
    
    //~ Gpu Resource Types
    
    typedef struct mp_command_pool*           command_pool;
    typedef struct mp_command_list*           command_list;
    
    typedef struct mp_pipeline*               pipeline;
    
    typedef struct mp_render_component*       render_component;  
    typedef struct mp_upload_buffer*          upload_buffer;
    typedef struct mp_image*                  image;
    
    typedef struct mp_uniform_buffer*         uniform_buffer;
    // TODO(Dustin): Expose in the future
    typedef struct mp_dynamic_uniform_buffer* dynamic_uniform_buffer;
    
    typedef struct mp_descriptor_pool*         descriptor_pool;
    typedef struct mp_descriptor_layout*       descriptor_layout;
    typedef struct mp_descriptor_set*          descriptor_set;
    
    //~ Create Info Structs
    
    typedef struct graphics_create_info 
    {
        window_t  Window;
        u32       WindowWidth;
        u32       WindowHeight;
        platform *Platform;
    } graphics_create_info;
    
    typedef enum render_mode
    {
        RenderMode_Solid     = BIT(0),
        RenderMode_Wireframe = BIT(1),
        RenderMode_NormalVis = BIT(2),
    } render_mode;
    
    typedef struct command_pool_create_info
    {
        command_pool *CommandPool;
        
        // Maybe allow for AllocateBits to be set.
        // See Vulkan Docs: 
        // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkCommandPoolCreateFlagBits.html
        
    } command_pool_create_info;
    
    typedef struct command_list_create_info
    {
        command_list *CommandList;
        command_pool  CommandPool;
        
    } command_list_create_info;
    
    typedef struct begin_frame_cmd
    {
    } begin_frame_cmd;
    
    typedef struct end_frame_cmd
    {
        command_list *CommandList;
        u32           CommandListCount;
    } end_frame_cmd;
    
    typedef struct pipeline_create_info
    {
        char *VertexShader;
        char *FragmentShader;
        char *GeometryShader;
        char *TessControlShader;
        char *TessEvalShader;
        
        VkPipelineVertexInputStateCreateInfo VertexInputInfo;
        
        VkViewport            *Viewport;
        VkRect2D              *Scissor;
        u32                    ScissorCount;
        u32                    ViewportCount;
        
        // Input assembly
        VkPrimitiveTopology    Topology;
        
        // rasterizer
        VkPolygonMode          PolygonMode;
        VkFrontFace            FrontFace;
        r32                    LineWidth;
        
        // Multisample
        bool                   HasMultisampling;
        VkSampleCountFlagBits  MuliSampleSamples;
        
        // Depth/Stencil
        bool HasDepthStencil;
        
        // LayoutCreateInfo
        VkPushConstantRange   *PushConstants;
        u32                    PushConstantsCount;
        
        // TODO(Dustin): How to handle descriptor layouts??
        // Should they be a resource or should they be created within
        // the pipeline?
        descriptor_layout     *DescriptorLayouts;
        u32                    DescriptorLayoutsCount;
        
        VkRenderPass           RenderPass;
        
        // TODO(Dustin): Might want to expose subpasses?
    } pipeline_create_info;
    
    typedef struct render_component_create_info
    {
        void *VertexData;
        u32   VertexCount;
        u32   VertexStride;
        
        bool  HasIndices;
        void *IndexData;
        u32   IndexCount;
        u32   IndexStride;
    } render_component_create_info;
    
    typedef enum upload_buffer_type
    {
        UploadBuffer_Vertex, // create a vertex upload buffer
        UploadBuffer_Index,  // create an index upload buffer
        UploadBuffer_Texture,
    } upload_buffer_type;
    
    typedef struct upload_buffer_info
    {
        u64                Size;
        upload_buffer_type Type;
    } upload_buffer_info;
    
    typedef struct image_create_info
    {
        // Image/Buffer Info
        u32 Width;
        u32 Height;
        u32 MipLevels;
        
        // Image View Info
        VkFormat ImageFormat;
        
        // Sampler Info
        VkFilter MagFilter;
        VkFilter MinFilter;
        VkSamplerAddressMode AddressModeU;
        VkSamplerAddressMode AddressModeV;
        VkSamplerAddressMode AddressModeW;
        
    } image_create_info;
    
    
    typedef struct descriptor_layout_create_info
    {
        VkDescriptorSetLayoutBinding *Bindings;
        u32                           BindingsCount;
    } descriptor_layout_create_info;
    
    typedef struct descriptor_set_create_info
    {
        descriptor_layout Layout;
        u32               Set;
        u32               Binding;
    } descriptor_set_create_info;
    
    typedef struct descriptor_write_info
    {
        // Buffer information
        // TODO(Dustin): Account for uniform and dyn. uniform buffers 
        image Image;
        u32   Offset;
        u64   Range;
    } descriptor_write_info;
    
    //~ API Functions
    
#define INITIALIZE_GRAPHICS(fn) EXTERN_GRAPHICS_API void fn(graphics_create_info *CreateInfo) 
    typedef void (GRAPHICS_CALL *PFN_initialize_graphics)(graphics_create_info *CreateInfo); 
    
#define SHUTDOWN_GRAPHICS(fn) EXTERN_GRAPHICS_API void fn()
    typedef void (GRAPHICS_CALL *PFN_shutdown_graphics)();
    
#define SET_RENDER_MODE(fn) EXTERN_GRAPHICS_API void fn(render_mode Mode)
    typedef void (GRAPHICS_CALL *PFN_set_render_mode)();
    
#define GET_RENDER_MODE(fn) EXTERN_GRAPHICS_API u32 fn()
    typedef u32 (GRAPHICS_CALL *PFN_get_render_mode)();
    
#define BEGIN_FRAME(fn) EXTERN_GRAPHICS_API void fn()
    typedef void (GRAPHICS_CALL *PFN_begin_frame)();
    
#define END_FRAME(fn) EXTERN_GRAPHICS_API void fn(end_frame_cmd *EndFrameInfo)
    typedef void (GRAPHICS_CALL *PFN_end_frame)(end_frame_cmd *EndFrameInfo);
    
#define WAIT_FOR_LAST_FRAME(fn) EXTERN_GRAPHICS_API void fn()
    typedef void (GRAPHICS_CALL *PFN_wait_for_last_frame)();
    
#define CREATE_COMMAND_POOL(fn) EXTERN_GRAPHICS_API void fn(command_pool_create_info *CreateInfo)
    typedef void (GRAPHICS_CALL *PFN_create_command_pool)(command_pool_create_info *CreateInfo);
    
#define FREE_COMMAND_POOL(fn) EXTERN_GRAPHICS_API void fn(command_pool *CommandPool)
    typedef void (GRAPHICS_CALL *PFN_free_command_pool)(command_pool *CommandPool);
    
#define CREATE_COMMAND_LIST(fn) EXTERN_GRAPHICS_API void fn(command_list_create_info *CreateInfo)
    typedef void (GRAPHICS_CALL *PFN_create_command_list)(command_list_create_info *CreateInfo);
    
#define FREE_COMMAND_LIST(fn) EXTERN_GRAPHICS_API void fn(command_list *CommandList)
    typedef void (GRAPHICS_CALL *PFN_free_command_list)(command_list *CommandList);
    
#define EXECUTE_COMMAND_LIST(fn) EXTERN_GRAPHICS_API void fn(command_list CommandList)
    typedef void (GRAPHICS_CALL *PFN_execute_command_list)(command_list CommandList);
    
#define CREATE_PIPELINE(fn) EXTERN_GRAPHICS_API void fn(pipeline_create_info *PipelineInfo, pipeline *Pipeline)
    typedef void (GRAPHICS_CALL *PFN_create_pipeline)(pipeline_create_info *PipelineInfo, pipeline *Pipeline);
    
#define FREE_PIPELINE(fn) EXTERN_GRAPHICS_API void fn(pipeline *Pipeline)
    typedef void (GRAPHICS_CALL *PFN_free_pipeline)(pipeline *Pipeline);
    
#define CREATE_RENDER_COMPONENT(fn) EXTERN_GRAPHICS_API void fn(render_component_create_info *RenderInfo, \
    render_component *RenderComponent)
        typedef void (GRAPHICS_CALL *PFN_create_render_component)(render_component_create_info *RenderInfo,
                                                                  render_component *RenderComponent);
    
#define FREE_RENDER_COMPONENT(fn) EXTERN_GRAPHICS_API void fn(render_component *RenderComponent)
    typedef void (GRAPHICS_CALL *PFN_free_render_component)(render_component *RenderComponent);
    
#define SET_RENDER_COMPONENT_INFO(fn) EXTERN_GRAPHICS_API void fn(render_component RenderComponent, bool IsIndexed, VkIndexType IndexType, u32 DrawCount)
    typedef void (GRAPHICS_CALL *PFN_set_render_component_info)(render_component RenderComponent, bool IsIndexed, VkIndexType IndexType, u32 DrawCount);
    
    // Upload buffer, used as a "staging buffer" for gpu-only buffer.
    
#define CREATE_UPLOAD_BUFFER(fn) EXTERN_GRAPHICS_API void fn(upload_buffer *Buffer, upload_buffer_type BufferType, u64 BufferSize)
    typedef void (GRAPHICS_CALL *PFN_create_upload_buffer)(upload_buffer *Buffer, upload_buffer_type BufferType, u64 BufferSize);
    
#define UPDATE_UPLOAD_BUFFER(fn)  EXTERN_GRAPHICS_API void fn(upload_buffer UploadBuffer, void *Data, u64 Size, u64 Offset)
    typedef void (GRAPHICS_CALL *PFN_update_upload_buffer)(upload_buffer UploadBuffer, void *Data, u64 Size, u64 Offset);
    
#define MAP_UPLOAD_BUFFER(fn) EXTERN_GRAPHICS_API void fn(upload_buffer UploadBuffer, u64 Offset, void **Ptr)
    typedef void (GRAPHICS_CALL *PFN_map_upload_buffer)(upload_buffer UploadBuffer, u64 Offset, void **Ptr);
    
#define UNMAP_UPLOAD_BUFFER(fn) EXTERN_GRAPHICS_API void fn(upload_buffer UploadBuffer, void **Ptr)
    typedef void (GRAPHICS_CALL *PFN_unmap_upload_buffer)(upload_buffer UploadBuffer, void **Ptr);
    
#define RESIZE_UPLOAD_BUFFER(fn) EXTERN_GRAPHICS_API void fn(upload_buffer Buffer, u64 NewSize)
    typedef void (GRAPHICS_CALL *PFN_resize_upload_buffer)(upload_buffer Buffer, u64 NewSize);
    
#define COPY_UPLOAD_BUFFER(fn) EXTERN_GRAPHICS_API void fn(upload_buffer UploadBuffer, render_component RenderComponent)
    typedef void (GRAPHICS_CALL *PFN_copy_upload_buffer)(upload_buffer Buffer, render_component RenderComponent);
    
#define GET_UPLOAD_BUFFER_INFO(fn) EXTERN_GRAPHICS_API upload_buffer_info fn(upload_buffer UploadBuffer)
    typedef upload_buffer_info (GRAPHICS_CALL *PFN_get_upload_buffer_info)(upload_buffer UploadBuffer);
    
#define FREE_UPLOAD_BUFFER(fn) EXTERN_GRAPHICS_API void fn(upload_buffer *Buffer)
    typedef void (GRAPHICS_CALL *PFN_free_upload_buffer)(upload_buffer *Buffer);
    
    // Images
    
#define CREATE_IMAGE(fn) EXTERN_GRAPHICS_API void fn(image *Image, image_create_info *ImageInfo) 
    typedef void (GRAPHICS_CALL *PFN_create_image)(image *Image, image_create_info *ImageInfo);
    
#define FREE_IMAGE(fn) EXTERN_GRAPHICS_API void fn(image *Image) 
    typedef void (GRAPHICS_CALL *PFN_free_image)(image *Image);
    
#define COPY_BUFFER_TO_IMAGE(fn) EXTERN_GRAPHICS_API void fn(image Image, upload_buffer UploadBuffer)
    typedef void (GRAPHICS_CALL *PFN_copy_buffer_to_image)(image Image, upload_buffer UploadBuffer);
    
#define GET_IMAGE_DIMENSIONS(fn) EXTERN_GRAPHICS_API void fn(image Image, u32 *Width, u32 *Height)
    typedef void (GRAPHICS_CALL *PFN_get_image_dimensions)(image Image, u32 *Width, u32 *Height);
    
#define RESIZE_IMAGE(fn) EXTERN_GRAPHICS_API void fn(image Image, u32 Width, u32 Height) 
    typedef void (GRAPHICS_CALL *PFN_resize_image)(image Image, u32 Width, u32 Height);
    
    // Descriptors 
    
#define CREATE_DESCRIPTOR_SET_LAYOUT(fn) EXTERN_GRAPHICS_API void fn(descriptor_layout *Layout, descriptor_layout_create_info *LayoutInfo) 
    typedef void (GRAPHICS_CALL *PFN_create_descriptor_set_layout)(descriptor_layout *Layout, descriptor_layout_create_info *LayoutInfo);
    
#define FREE_DESCRIPTOR_SET_LAYOUT(fn) EXTERN_GRAPHICS_API void fn(descriptor_layout *Layout) 
    typedef void (GRAPHICS_CALL *PFN_free_descriptor_set_layout)(descriptor_layout *Layout);
    
#define CREATE_DESCRIPTOR_SET(fn) EXTERN_GRAPHICS_API void fn(descriptor_set *Set, descriptor_set_create_info *SetInfo) 
    typedef void (GRAPHICS_CALL *PFN_create_descriptor_set)(descriptor_set *Set, descriptor_set_create_info *SetInfo);
    
#define FREE_DESCRIPTOR_SET(fn) EXTERN_GRAPHICS_API void fn(descriptor_set *Set) 
    typedef void (GRAPHICS_CALL *PFN_free_descriptor_set)(descriptor_set *Set);
    
#define BIND_BUFFER_TO_DESCRIPTOR_SET(fn) EXTERN_GRAPHICS_API void fn(descriptor_set Set, descriptor_write_info *WriteInfo)
    typedef void (GRAPHICS_CALL *PFN_bind_buffer_to_descriptor_set)(descriptor_set Set, descriptor_write_info *WriteInfo);
    
    
    // Command List Commands
    
#define CMD_BIND_PIPELINE(fn) EXTERN_GRAPHICS_API void fn(pipeline Pipeline, command_list CommandList)
    typedef void (GRAPHICS_CALL *PFN_cmd_bind_pipeline)(pipeline Pipeline, command_list CommandList);
    
#define CMD_DRAW(fn) EXTERN_GRAPHICS_API void fn(render_component RenderComponent, command_list CommandList)
    typedef void (GRAPHICS_CALL *PFN_cmd_draw)(render_component RenderComponent, command_list CommandList);
    
#define CMD_SET_CAMERA_DATA(fn) EXTERN_GRAPHICS_API void fn(command_list CommandList, mat4 LookAt, mat4 Projection)
    typedef void (GRAPHICS_CALL *PFN_cmd_set_camera_data)(command_list CommandList, mat4 LookAt, mat4 Projection);
    
#define CMD_SET_OBJECT_WORLD_DATA(fn) EXTERN_GRAPHICS_API void fn(command_list CommandList, vec3 Position, vec3 Scale, quaternion Rotation)
    typedef void (GRAPHICS_CALL *PFN_cmd_set_object_world_data)(command_list CommandList, vec3 Position, vec3 Scale, quaternion Rotation);
    
#define CMD_SET_CAMERA(fn) EXTERN_GRAPHICS_API void fn(command_list CommandList, mat4 Projection, mat4 View)
    typedef void (GRAPHICS_CALL *PFN_cmd_set_camera)(command_list CommandList, mat4 Projection, mat4 View);
    
    // TODO(Dustin): Bind Descriptor
    
#define CMD_BIND_DESCRIPTOR_SET(fn) EXTERN_GRAPHICS_API void fn(command_list CommandList, descriptor_set Set)
    typedef void (GRAPHICS_CALL *PFN_cmd_bind_descriptor_set)(command_list CommandList, descriptor_set Set);
    
    
    
#ifdef __cplusplus
} // extern "C"
#endif


#endif //GRAPHICS_MAPLE_GRAPHICS_H
