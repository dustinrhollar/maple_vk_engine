#ifndef GRAPHICS_RENDERER_H
#define GRAPHICS_RENDERER_H

typedef struct object_data
{
    vec3       Position;
    vec3       Scale;
    quaternion Rotation;
} object_data;

typedef struct object_data_buffer
{
    VkDescriptorSetLayout     DescriptorLayout;
    
    VkDescriptorSet          *DescriptorSets;
    u32                       DescriptorSetsCount;
    
    mp_dynamic_uniform_buffer Buffer;
} object_data_buffer;

typedef struct camera_data
{
    alignas(16) mat4 View;
    alignas(16) mat4 Projection;
} camera_data;

typedef struct global_shader_data
{
    VkDescriptorSetLayout     DescriptorLayout;
    
    VkDescriptorSet          *DescriptorSets;
    u32                       DescriptorSetsCount;
    
    mp_uniform_buffer Buffer;
} global_shader_data;

typedef struct renderer
{
    //~ Render Settings
    
    u32 RenderMode; // Bitfield for render settings
    
    //~ Frame State
    
    VkRenderPass     PrimaryRenderPass;
    VkCommandPool    CommandPool;
    
    VkFramebuffer   *Framebuffers;
    u32              FramebufferCount;
    // TODO(Dustin): Sample Count for MSAA
    image_parameters DepthResources;
    
    VkCommandBuffer *CommandBuffers;
    u32              CommandBuffersCount;
    
    // Global Buffers/Descriptors
    VkDescriptorPool DescriptorPool;
    
    // single uniform buffer to hold the View/Project Matrices
    
    // Pre-Frame info
    u32                 CurrentImageIndex;
    VkCommandBuffer    *ActiveCommandBuffer;
    struct mp_pipeline *ActivePipeline;
    camera_data         ActiveCamera;
    
    // For now, contains the View/Projection matrix
    // updated via "cmd_set_camera" function
    global_shader_data  GlobalShaderData;
    object_data_buffer  ObjectDataBuffer;
    
    // TODO(Dustin): Maintain a list of resizable resources...?
    
} renderer;

void renderer_init(renderer *Renderer);
void renderer_free(renderer *Renderer);
u32 renderer_begin_frame();
void renderer_end_frame();

void object_data_buffer_init(object_data_buffer *ObjectData);
void object_data_buffer_free(object_data_buffer *ObjectData);
void object_data_buffer_begin_frame(object_data_buffer *ObjectData);
void object_data_buffer_end_frame(object_data_buffer *ObjectData);
void object_data_buffer_update(object_data_buffer *ObjectData);

void global_shader_data_init(global_shader_data *ShaderData);
void global_shader_data_free(global_shader_data *ShaderData);
void global_shader_data_update(global_shader_data *ShaderData);


#endif //RENDERER_H
