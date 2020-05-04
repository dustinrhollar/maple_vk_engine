#ifndef SPLICER_ENGINE_VULKAN_FUNCTIONS_H
#define SPLICER_ENGINE_VULKAN_FUNCTIONS_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif
    
    namespace vk {
        
#define VK_EXPORTED_FUNCTION(fun)                                 extern PFN_##fun fun;
#define VK_GLOBAL_LEVEL_FUNCTION(fun)                             extern PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION(fun)                           extern PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION_FROM_EXTENSION(fun, extension) extern PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION(fun)                             extern PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION_FROM_EXTENSION(fun, extension)   extern PFN_##fun fun;
        
#include "list_of_functions.inl"
        
    };
    
#ifdef __cplusplus
}
#endif

#endif //SPLICER_ENGINE_VULKAN_FUNCTIONS_H
