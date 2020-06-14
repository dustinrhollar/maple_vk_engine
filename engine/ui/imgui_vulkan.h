#ifndef ENGINE_UI_IMGUI_VULKAN_H
#define ENGINE_UI_IMGUI_VULKAN_H

struct maple_dev_gui;
#define DEV_UI_DRAW_CALLBACK(fn) void fn(maple_dev_gui *DevGui, VkCommandBuffer CommandBuffer)
typedef DEV_UI_DRAW_CALLBACK(dev_ui_draw_callback);

struct dev_ui_push_constant_block
{
    vec2 Scale;
    vec2 Translate;
};

struct maple_dev_gui
{
    u32                         SwapImageCount;
    u32                         ImageIndex; // the current image index
    
    ImageParameters             FontImage;
    
    BufferParameters           *VertexBuffer;
    BufferParameters           *IndexBuffer;
    
    i32                         VertexCount;
    i32                         IndexCount;
    
    VkPipelineCache             PipelineCache;
    VkPipelineLayout            PipelineLayout;
    VkPipeline                  Pipeline;
    
    VkCommandPool               CommandPool;
    
    VkDescriptorPool            DescriptorPool;
    VkDescriptorSetLayout       DescriptorLayout;
    VkDescriptorSet             DescriptorSet;
    
    // Callback function
    dev_ui_draw_callback       *Draw;
};

void MapleDevGuiInit(maple_dev_gui *DevGui, VkRenderPass RenderPass, dev_ui_draw_callback *Draw);
void MapleDevGuiFree(maple_dev_gui *DevGui);
void MapleDevGuiDraw(maple_dev_gui *DevGui, VkCommandBuffer CommandBuffer);

#endif //ENGINE_UI_IMGUI_VULKAN_H
