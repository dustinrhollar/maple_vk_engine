
//~ Debug State

file_global u32 GlobalValidationCount = 2;
file_global const char *GlobalValidationLayers[] = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_NV_optimus",
};

file_global u32 GlobalDeviceExtensionsCount = 1;
file_global const char *GlobalDeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


#ifdef NDEBUG
file_global const bool GlobalEnabledValidationLayers = false;
#else
file_global const bool GlobalEnabledValidationLayers = true;
#endif

file_global VkDebugUtilsMessengerEXT GlobalDebugMessenger;


//~ Vulkan Library functions lie within the vk namespace
// TODO(Dustin): Remove the use of std::string -> can use mstring now...

namespace vk
{
    file_internal bool LoadVulkanLibrary()
    {
#ifdef VK_USE_PLATFORM_WIN32_KHR
        VulkanLibrary = LoadLibrary("vulkan-1.dll");
#elif defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR)
        VulkanLibrary = dlopen("libvulkan.so", RTLD_LAZY);
#endif
        
        if (VulkanLibrary == nullptr)
        {
            mprinte("Unable to load Vulkan library!\n");
            return false;
        }
        
        return true;
    }
    
    
    file_internal bool LoadExportedFunctions()
    {
#ifdef VK_USE_PLATFORM_WIN32_KHR
#define LoadProcAddress GetProcAddress
#else
#define LoadProcAddress
#endif
        
#define VK_EXPORTED_FUNCTION(fun)                                    \
        if (!(fun = (PFN_##fun)LoadFunction(VulkanLibrary, #fun))) {     \
                     mprinte("Could not load exported function: %s\n", #fun);      \
                             return false;                                                \
    }
    
#include "list_of_functions.inl"
    
    return true;
}

file_internal bool LoadExportedEntryPoints()
{
#ifdef VK_USE_PLATFORM_WIN32_KHR
#define LoadProcAddress GetProcAddress
#else
#define LoadProcAddress
#endif
    
#define VK_EXPORTED_FUNCTION(fun)                                       \
    if (!(fun = (PFN_##fun)LoadFunction(VulkanLibrary, #fun))) {     \
                 mprinte("Could not load exported function: %s\n", #fun);         \
                         return false;                                                   \
}

#include "list_of_functions.inl"

return true;
}


file_internal bool LoadGlobalLevelEntryPoints()
{
#define VK_GLOBAL_LEVEL_FUNCTION(fun) \
    if (!(fun = (PFN_##fun)vkGetInstanceProcAddr(nullptr, #fun))) {     \
                 mprinte("Could not load global level function: %s!\n", #fun);    \
                         return false;                                                   \
}

#include "list_of_functions.inl"

return true;
}

file_internal bool LoadInstanceLevelEntryPoints(VkInstance instance,
                                                char const **enabled_extensions,
                                                u32 ExtensionCount)
{
#define VK_INSTANCE_LEVEL_FUNCTION(fun)                                \
    if (!(fun = (PFN_##fun)vkGetInstanceProcAddr(instance, #fun))) {   \
                 mprinte("Could not load instance level function: %s!\n", #fun); \
                         return false;                                                  \
}

#define VK_INSTANCE_LEVEL_FUNCTION_FROM_EXTENSION(fun, extension)                \
for (u32 i = 0; i < ExtensionCount; ++i) {                               \
    if (std::string(enabled_extensions[i]) == std::string(extension)) {  \
        if (!(fun = (PFN_##fun)vkGetInstanceProcAddr(instance, #fun))) { \
                     mprinte("Could not load instance level function from extension: %s!\n", #fun); \
                             return false;                                           \
    }                                                           \
}                                                               \
}

#include "list_of_functions.inl"

return true;
}

file_internal bool LoadDeviceLevelEntryPoints(VkDevice logical_device,
                                              char const **enabled_extensions,
                                              u32 ExtensionCount)
{
#define VK_DEVICE_LEVEL_FUNCTION(fun)                                    \
    if (!(fun = (PFN_##fun)vkGetDeviceProcAddr(logical_device, #fun))) { \
                 mprinte("Could not load device level function: %s!\n", #fun);    \
                         return false;                                                   \
}

#define VK_DEVICE_LEVEL_FUNCTION_FROM_EXTENSION(fun, extension)         \
for (u32 i = 0; i < ExtensionCount; ++i) {               \
    if (std::string(enabled_extensions[i]) == std::string(extension)) { \
        if (!(fun = (PFN_##fun)vkGetDeviceProcAddr(logical_device, #fun))) { \
                     mprinte("Could not load device level function from extension: %s!\n", #fun); \
                             return false;                                           \
    }                                                           \
}                                                               \
}

#include "list_of_functions.inl"

return true;
}


file_internal bool CheckValidationLayerSupport()
{
    u32 layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    VkLayerProperties *availableLayers = palloc<VkLayerProperties>(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
    
    for (u32 i = 0; i < GlobalValidationCount; ++i)
    {
        bool layerFound = false;
        
        for (u32 j = 0; j < layerCount; ++j)
        {
            if (strcmp(GlobalValidationLayers[i], availableLayers[j].layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        
        if (!layerFound)
        {
            return false;
        }
    }
    
    pfree(availableLayers);
    
    return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData)
{
    
    //PlatformPrintMessage(ConsoleColor_Yellow, ConsoleColor_DarkGrey, "%s\n", pCallbackData->pMessage);
    mprinte("%s\n", pCallbackData->pMessage);
    
    return VK_FALSE;
}

// TODO(Dustin): Load this through the loader like all other functions
file_internal VkResult CreateDebugUtilsMessengerEXT(VkInstance Instance,
                                                    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                    const VkAllocationCallbacks* pAllocator,
                                                    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(Instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// TODO(Dustin): Load this through the loader like all other functions
file_internal void DestroyDebugUtilsMessengerEXT(VkInstance Instance,
                                                 VkDebugUtilsMessengerEXT debugMessenger,
                                                 const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(Instance, debugMessenger, pAllocator);
    }
}


file_internal void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

file_internal void SetupDebugMessenger(VkInstance Instance)
{
    if (!GlobalEnabledValidationLayers) return;
    
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    PopulateDebugMessengerCreateInfo(createInfo);
    
    VK_CHECK_RESULT(CreateDebugUtilsMessengerEXT(Instance, &createInfo, nullptr, &GlobalDebugMessenger),
                    "Failed to set up debug messenger!");
}

};

//~ Vulkan Core function defs

bool vulkan_core::Init()
{
    // load the Vulkan Library
    if (!vk::LoadVulkanLibrary())
        return false;
    
    // Load export function
    if (!vk::LoadExportedEntryPoints())
        return false;
    
    // Load Global Entry functions
    if (!vk::LoadGlobalLevelEntryPoints())
        return false;
    
    if (!CreateInstance())
        return false;
    
    vk::SetupDebugMessenger(Instance);
    
    
    // Load Instance Level Functions
    const char *instance_extensions[2];
    
    const char *khr_surface_name = VK_KHR_SURFACE_EXTENSION_NAME;
    
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    const char *plat_surface     = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    const char *plat_surface     = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    const char *plat_surface     = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
#else
#error Unknown Platform definiation for creating Vulkan Surface
#endif
    
    instance_extensions[0] = khr_surface_name;
    instance_extensions[1] = plat_surface;
    
    if (!vk::LoadInstanceLevelEntryPoints(Instance, instance_extensions, 2))
        return false;
    
    PresentationSurface = VK_NULL_HANDLE;
    PlatformVulkanCreateSurface(&PresentationSurface, Instance);
    assert(PresentationSurface != VK_NULL_HANDLE);
    
    PickPhysicalDevice(Instance);
    CreateLogicalDevice();
    
    // Load all Device related functions
    const char *device_extensions[1];
    
    const char *khr_swapchain_name = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    
    device_extensions[0] = khr_swapchain_name;
    
#if 1
    if (!vk::LoadDeviceLevelEntryPoints(Device, device_extensions, 1))
        return false;
#endif
    
    // Retrive the Graphics and Present Queue from the logical device
    {
        queue_family_indices indices = FindQueueFamilies(PhysicalDevice);
        
        GraphicsQueue = {};
        PresentQueue = {};
        
        vk::vkGetDeviceQueue(Device, indices.graphicsFamily.value(), 0,
                             &GraphicsQueue.Handle);
        vk::vkGetDeviceQueue(Device, indices.presentFamily.value(), 0,
                             &PresentQueue.Handle);
        
        GraphicsQueue.FamilyIndex = indices.graphicsFamily.value();
        PresentQueue.FamilyIndex  = indices.presentFamily.value();
    }
    
    CreateSwapchain(SwapChain);
    CreateSyncObjects(SyncObjects);
    
    // Setup Vulkan Proxy Allocator
    //mm::Allocator *GlobalPermanantStorage = mm::GetPermanantStorage();
    //void *ptr = (void*)palloc<jengine::mm::VulkanProxyAllocator>(1);
    //GlobalVulkanState.VkProxyAllocator = new (ptr) jengine::mm::VulkanProxyAllocator(*GlobalPermanantStorage);
    
    //GlobalVulkanState.VkProxyAllocator = jengin::mm::VulkanProxyAllocator(*GlobalPermanantStorage);
    
    VmaAllocatorCreateInfo alloc_info = {};
    alloc_info.physicalDevice = PhysicalDevice;
    alloc_info.device = Device;
    alloc_info.instance = Instance;
    
    // Might have to heap allocate?
    VmaVulkanFunctions vmaf= {};
    vmaf.vkGetPhysicalDeviceProperties = vk::vkGetPhysicalDeviceProperties;
    vmaf.vkGetPhysicalDeviceProperties = vk::vkGetPhysicalDeviceProperties;
    vmaf.vkGetPhysicalDeviceMemoryProperties = vk::vkGetPhysicalDeviceMemoryProperties;
    vmaf.vkAllocateMemory = vk::vkAllocateMemory;
    vmaf.vkFreeMemory = vk::vkFreeMemory;
    vmaf.vkMapMemory = vk::vkMapMemory;
    vmaf.vkUnmapMemory = vk::vkUnmapMemory;
    vmaf.vkFlushMappedMemoryRanges = vk::vkFlushMappedMemoryRanges;
    vmaf.vkInvalidateMappedMemoryRanges = vk::vkInvalidateMappedMemoryRanges;
    vmaf.vkBindBufferMemory = vk::vkBindBufferMemory;
    vmaf.vkBindImageMemory = vk::vkBindImageMemory;
    vmaf.vkGetBufferMemoryRequirements = vk::vkGetBufferMemoryRequirements;
    vmaf.vkGetImageMemoryRequirements = vk::vkGetImageMemoryRequirements;
    vmaf.vkCreateBuffer = vk::vkCreateBuffer;
    vmaf.vkDestroyBuffer = vk::vkDestroyBuffer;
    vmaf.vkCreateImage = vk::vkCreateImage;
    vmaf.vkDestroyImage = vk::vkDestroyImage;
    vmaf.vkCmdCopyBuffer = vk::vkCmdCopyBuffer;
#if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
    //vmaf.vkGetBufferMemoryRequirements2KHR = vk::vkGetBufferMemoryRequirements2KHR;
    //vmaf.vkGetImageMemoryRequirements2KHR  = vk::vkGetImageMemoryRequirements2KHR;
#endif
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
    //vmaf.vkBindBufferMemory2KHR = vk::vkBindBufferMemory2KHR;
    //vmaf.vkBindImageMemory2KHR  = vk::vkBindImageMemory2KHR;
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
    //vmaf.vkGetPhysicalDeviceMemoryProperties2KHR = vk::vkGetPhysicalDeviceMemoryProperties2KHR;
#endif
    
    alloc_info.pVulkanFunctions = &vmaf;
    //alloc_info.pAllocationCallbacks = &GlobalVulkanState.VkProxyAllocator->GetVkAllocationCallbacks();
    
    vmaCreateAllocator(&alloc_info, &VulkanAllocator);
    
    return true;
}

void vulkan_core::Shutdown()
{
    vmaDestroyAllocator(VulkanAllocator);
    
    for (int i = 0; i < sync_object_parameters::MAX_FRAMES; ++i)
    {
        vk::vkDestroySemaphore(Device, SyncObjects.RenderFinished[i], nullptr);
        vk::vkDestroySemaphore(Device, SyncObjects.ImageAvailable[i], nullptr);
        vk::vkDestroyFence(Device,     SyncObjects.InFlightFences[i], nullptr);
    }
    
    
    // Free the swapchain
    for (u32 i = 0; i < SwapChain.ImagesCount; ++i) {
        image_parameters iparam = SwapChain.Images[i];
        vk::vkDestroyImageView(Device, iparam.View, nullptr);
    }
    //GlobalVulkanState.SwapChain.Images.Resize(0);
    pfree(SwapChain.Images);
    SwapChain.ImagesCount = 0;
    
    vk::vkDestroySwapchainKHR(Device, SwapChain.Handle, nullptr);
    
    vk::vkDestroyDevice(Device, nullptr);
    if (GlobalEnabledValidationLayers) {
        vk::DestroyDebugUtilsMessengerEXT(Instance, GlobalDebugMessenger, nullptr);
    }
    
    vk::vkDestroySurfaceKHR(Instance, PresentationSurface, nullptr);
    vk::vkDestroyInstance(Instance, nullptr);
}


image_parameters* vulkan_core::GetSwapChainImages()
{
    return SwapChain.Images;
}

u32 vulkan_core::GetSwapChainImageCount() {
    return SwapChain.ImagesCount;
}


VkFormat vulkan_core::GetSwapChainImageFormat() {
    return SwapChain.Format;
}

VkExtent2D vulkan_core::GetSwapChainExtent() {
    return SwapChain.Extent;
}


bool vulkan_core::CreateInstance()
{
    if (GlobalEnabledValidationLayers && !vk::CheckValidationLayerSupport())
    {
        mprinte("Validation layers requested, but not available!");
        return false;
    }
    
    VkApplicationInfo appInfo  = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Maple Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "Maple";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo     = &appInfo;
    
    // TODO(Dustin): Moveto using an array rather than a vector
    // Vulkan creates an extension interface to interact with the window api
    // glfw has a handy way of obtaining the extensions for the platform
    
    const char* exts[3];
    u32 ExtsCount = 0;
    
    const char *surface_exts = "VK_KHR_surface";
    const char* plat_exts = PlatformGetRequiredInstanceExtensions(GlobalEnabledValidationLayers);
    
    exts[0] = surface_exts;
    exts[1] = plat_exts;
    ExtsCount = 2;
    
    if (GlobalValidationLayers)
    {
        const char *name = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        exts[2] = name;
        
        ExtsCount++;
    }
    
    createInfo.enabledExtensionCount   = ExtsCount;
    createInfo.ppEnabledExtensionNames = exts;
    
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (GlobalEnabledValidationLayers) {
        createInfo.enabledLayerCount   = GlobalValidationCount;
        createInfo.ppEnabledLayerNames = GlobalValidationLayers;
        
        vk::PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        
        createInfo.pNext = nullptr;
    }
    
    VK_CHECK_RESULT(vk::vkCreateInstance(&createInfo, nullptr, &Instance),
                    "Failed to create instance!");
    
    assert(Instance != NULL);
    
    return true;
}


queue_family_indices vulkan_core::FindQueueFamilies(VkPhysicalDevice physical_device)
{
    queue_family_indices indices;
    
    u32 queueFamilyCount = 0;
    vk::vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyCount, nullptr);
    
    VkQueueFamilyProperties *queueFamilies = palloc<VkQueueFamilyProperties>(queueFamilyCount);
    vk::vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyCount, queueFamilies);
    
    // Find a familiy with the graphics bit
    for (u32 i = 0; i < queueFamilyCount; ++i)
    {
        VkQueueFamilyProperties queueFamily = queueFamilies[i];
        
        if (queueFamily.queueCount > 0 &&
            queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }
        
        VkBool32 presentSupport = false;
        vk::vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i,
                                                 PresentationSurface,
                                                 &presentSupport);
        if (queueFamily.queueCount > 0 && presentSupport)
        {
            indices.presentFamily = i;
        }
        
        if (indices.isComplete())
        {
            break;
        }
    }
    
    pfree(queueFamilies);
    
    return indices;
}


bool vulkan_core::CheckDeviceExtensionSupport(VkPhysicalDevice physical_device)
{
    u32 extensionCount;
    vk::vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, nullptr);
    
    VkExtensionProperties *availableExtensions = palloc<VkExtensionProperties>(extensionCount);
    vk::vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, availableExtensions);
    
    // insert extensions into a set to make sure they are all unique
    std::set<std::string> requiredExtensions;
    for (u32 i = 0; i < GlobalDeviceExtensionsCount; ++i)
        requiredExtensions.insert(GlobalDeviceExtensions[i]);
    
    for (u32 i = 0; i < extensionCount; ++i)
    {
        VkExtensionProperties extension = availableExtensions[i];
        requiredExtensions.erase(extension.extensionName);
    }
    
    pfree(availableExtensions);
    
    return requiredExtensions.empty();
}

swapchain_support_details vulkan_core::QuerySwapchainSupport(VkPhysicalDevice physical_device)
{
    swapchain_support_details details;
    
    vk::vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device,
                                                  PresentationSurface,
                                                  &details.Capabilities);
    
    vk::vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device,
                                             PresentationSurface,
                                             &details.FormatsCount,
                                             nullptr);
    
    if (details.FormatsCount != 0)
    {
        details.Formats = palloc<VkSurfaceFormatKHR>(details.FormatsCount);
        vk::vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device,
                                                 PresentationSurface,
                                                 &details.FormatsCount,
                                                 details.Formats);
    }
    
    vk::vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device,
                                                  PresentationSurface,
                                                  &details.PresentModesCount,
                                                  nullptr);
    
    if (details.PresentModesCount != 0)
    {
        details.PresentModes = palloc<VkPresentModeKHR>(details.PresentModesCount);
        
        vk::vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device,
                                                      PresentationSurface,
                                                      &details.PresentModesCount,
                                                      details.PresentModes);
    }
    
    return details;
}

bool vulkan_core::IsDeviceSuitable(VkPhysicalDevice physical_device)
{
    queue_family_indices indices = FindQueueFamilies(physical_device);
    if (!indices.isComplete()) return false;
    
    bool extensionsSupported = CheckDeviceExtensionSupport(physical_device);
    if (!extensionsSupported) return false;
    
    // Query Swapchain support
    swapchain_support_details details = QuerySwapchainSupport(physical_device);
    {
        bool Result = false;
        if (details.FormatsCount != 0 && details.PresentModesCount != 0)
            Result = true;
        
        // TODO(Dustin): Get allocations in temp memory instead of global
        if (details.Formats)      pfree(details.Formats);
        if (details.PresentModes) pfree(details.PresentModes);
        
        if (Result) return true;
    }
    
    VkPhysicalDeviceFeatures supportedFeatures;
    vk::vkGetPhysicalDeviceFeatures(physical_device, &supportedFeatures);
    return (supportedFeatures.samplerAnisotropy);
}

void vulkan_core::PickPhysicalDevice(VkInstance instance)
{
    //vkEnumerateDeviceExtensionProperties
    
    u32 deviceCount = 0;
    vk::vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    
    if (deviceCount == 0)
    {
        mprinte("Failed to find a GPU with Vulkan support!\n");
        return;
    }
    
    VkPhysicalDevice *devices = palloc<VkPhysicalDevice>(deviceCount);
    vk::vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
    
    // Finds the first suitable device
    for (u32 i = 0; i < deviceCount; ++i)
    {
        VkPhysicalDevice device = devices[i];
        
        if (IsDeviceSuitable(device))
        {
            PhysicalDevice = device;
            MsaaSamples = GetMaxUsableSampleCount();
            break;
        }
    }
    
    pfree(devices);
    
    if (PhysicalDevice == VK_NULL_HANDLE)
    {
        mprinte("Failed to find a suitable GPU!\n");
        return;
    }
}

void vulkan_core::CreateLogicalDevice()
{
    queue_family_indices indices = FindQueueFamilies(PhysicalDevice);
    
    // TODO(Dustin): Remove set
    std::set<u32> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };
    
    VkDeviceQueueCreateInfo *queueCreateInfos = palloc<VkDeviceQueueCreateInfo>(uniqueQueueFamilies.size());
    
    float queuePriority = 1.0f;
    int i = 0;
    for (u32 queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[i++] = queueCreateInfo;
    }
    
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.fillModeNonSolid  = VK_TRUE;
    deviceFeatures.geometryShader    = VK_TRUE;
    deviceFeatures.wideLines         = VK_TRUE;
    
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.queueCreateInfoCount = (u32)uniqueQueueFamilies.size();
    createInfo.pEnabledFeatures = &deviceFeatures;
    
    // enable the swap chain
    createInfo.enabledExtensionCount   = GlobalDeviceExtensionsCount;
    createInfo.ppEnabledExtensionNames = GlobalDeviceExtensions;
    
    // enable validation layers
    if (GlobalEnabledValidationLayers) {
        createInfo.enabledLayerCount   = GlobalValidationCount;
        createInfo.ppEnabledLayerNames = GlobalValidationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }
    
#if 0
    VK_CHECK_RESULT(vk::vkCreateDevice(PhysicalDevice,
                                       &createInfo, nullptr,
                                       &Device),
                    "Failed to logical device!");
#else
    VkResult res = vk::vkCreateDevice(PhysicalDevice,
                                      &createInfo, nullptr,
                                      &Device);
#endif
    
    int apple = 0;
    
    pfree(queueCreateInfos);
}


VkSampleCountFlagBits vulkan_core::GetMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties = {};
    vk::vkGetPhysicalDeviceProperties(PhysicalDevice, &physicalDeviceProperties);
    
    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT)  { return VK_SAMPLE_COUNT_8_BIT;  }
    if (counts & VK_SAMPLE_COUNT_4_BIT)  { return VK_SAMPLE_COUNT_4_BIT;  }
    if (counts & VK_SAMPLE_COUNT_2_BIT)  { return VK_SAMPLE_COUNT_2_BIT;  }
    
    return VK_SAMPLE_COUNT_1_BIT;
}


void vulkan_core::CreateSwapchain(swapchain_parameters &swapchain_params)
{
    // Query Swapchain support
    swapchain_support_details details = QuerySwapchainSupport(PhysicalDevice);
    
    // Get the surface format
    VkSurfaceFormatKHR surface_format;
    {
        VkSurfaceFormatKHR *availableFormats = details.Formats;
        bool found = false;
        for (u32 i = 0; i < details.FormatsCount; ++i)
        {
            VkSurfaceFormatKHR availableFormat = availableFormats[i];
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                surface_format =  availableFormat;
                found = true;
                break;
            }
        }
        
        if (!found)
            surface_format = availableFormats[0];
    }
    
    // Get the present mode
    VkPresentModeKHR present_mode;
    {
        VkPresentModeKHR *availablePresentModes = details.PresentModes;
        bool found = false;
        for (u32 i = 0; i < details.PresentModesCount; ++i)
        {
            VkPresentModeKHR availablePresentMode = availablePresentModes[i];
            
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                present_mode = availablePresentMode;
                found = true;
                break;
            }
        }
        
        if (!found)
            present_mode = VK_PRESENT_MODE_FIFO_KHR;
    }
    
    // Get the window extent
    VkExtent2D extent;
    {
        VkSurfaceCapabilitiesKHR *capabilities = &details.Capabilities;
        if (capabilities->currentExtent.width != UINT32_MAX)
        {
            extent = capabilities->currentExtent;
        }
        else
        {
            u32 width, height;
            PlatformGetClientWindowDimensions(&width, &height);
            VkExtent2D actualExtent = {(u32)width, (u32)height};
            
#define MAX(a, b) ((a) > (b)) ? a : b
#define MIN(a, b) ((a) < (b)) ? a : b
            actualExtent.width = MAX(capabilities->minImageExtent.width,
                                     MIN(capabilities->maxImageExtent.width, actualExtent.width));
            actualExtent.height = MAX(capabilities->minImageExtent.height,
                                      MIN(capabilities->maxImageExtent.height, actualExtent.height));
#undef MAX
#undef MIN
            
            extent = actualExtent;
        }
    }
    
    swapchain_params.Format = surface_format.format;
    swapchain_params.Extent = extent;
    
    u32 imageCount = details.Capabilities.minImageCount + 1;
    if (details.Capabilities.maxImageCount > 0 &&
        imageCount > details.Capabilities.maxImageCount)
    {
        imageCount = details.Capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = PresentationSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surface_format.format;
    createInfo.imageColorSpace = surface_format.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    u32 queueFamilyIndices[] = {
        GraphicsQueue.FamilyIndex,
        PresentQueue.FamilyIndex,
    };
    
    if (GraphicsQueue.FamilyIndex != PresentQueue.FamilyIndex)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    
    createInfo.preTransform = details.Capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = present_mode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    VK_CHECK_RESULT(vk::vkCreateSwapchainKHR(Device,
                                             &createInfo, nullptr,
                                             &swapchain_params.Handle),
                    "Failed to create the swap chain!");
    
    vk::vkGetSwapchainImagesKHR(Device, swapchain_params.Handle, &imageCount, nullptr);
    
    swapchain_params.ImagesCount = (imageCount);
    swapchain_params.Images = palloc<image_parameters>(imageCount);
    
    // @Cleanup: In order to get the swapchain images, need to alloc a temp
    // array to retrieve the images
    VkImage *images = palloc<VkImage>(imageCount);
    vk::vkGetSwapchainImagesKHR(Device, swapchain_params.Handle, &imageCount, images);
    
    for (u32 i = 0; i < imageCount; ++i)
    {
        image_parameters iparam = {};
        iparam.Handle = images[i];
        
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchain_params.Format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        iparam.View   = CreateImageView(viewInfo);
        
        swapchain_params.Images[i] = iparam;
    }
    
    pfree(images);
    
    // TODO(Dustin): Get allocations in temp memory instead of global
    if (details.Formats)      pfree(details.Formats);
    if (details.PresentModes) pfree(details.PresentModes);
}

void vulkan_core::CreateSyncObjects(sync_object_parameters &sync_objects)
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (int i = 0; i < sync_object_parameters::MAX_FRAMES; ++i)
    {
        VK_CHECK_RESULT(vk::vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &sync_objects.ImageAvailable[i]),
                        "Failed to create synchronization objects for a frame!");
        VK_CHECK_RESULT(vk::vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &sync_objects.RenderFinished[i]),
                        "Failed to create synchronization objects for a frame!");
        VK_CHECK_RESULT(vk::vkCreateFence(Device, &fenceInfo, nullptr, &sync_objects.InFlightFences[i]),
                        "Failed to create synchronization objects for a frame!");
    }
}

VkImageView vulkan_core::CreateImageView(VkImageViewCreateInfo create_info)
{
    VkImageView image_view;
    VK_CHECK_RESULT(vk::vkCreateImageView(Device, &create_info, nullptr, &image_view),
                    "Failed to create texture image view!");
    
    return image_view;
}


void vulkan_core::DestroyImageView(VkImageView image_view) 
{
    vk::vkDestroyImageView(Device, image_view, nullptr);
}

VkSampler vulkan_core::CreateImageSampler(VkSamplerCreateInfo sampler_info) 
{
    VkSampler sampler;
    VK_CHECK_RESULT(vk::vkCreateSampler(Device, &sampler_info, nullptr, &sampler),
                    "Could create image sampler!");
    
    return sampler;
}

void vulkan_core::DestroyImageSampler(VkSampler sampler) 
{
    vk::vkDestroySampler(Device, sampler, nullptr);
}

// @return false if the swapchain was out of date and a resize occured. true otherwise
VkResult vulkan_core::BeginFrame(u32 &next_image_idx)
{
    vk::vkWaitForFences(Device, 1,
                        &SyncObjects.InFlightFences[SyncObjects.CurrentFrame],
                        VK_TRUE, UINT64_MAX);
    
    // Draw frame
    VkResult khr_result = vk::vkAcquireNextImageKHR(Device,
                                                    SwapChain.Handle,
                                                    UINT64_MAX,
                                                    SyncObjects.ImageAvailable[SyncObjects.CurrentFrame],
                                                    VK_NULL_HANDLE,
                                                    &next_image_idx);
    
    return khr_result;
}

// @param current_frame_index, represents the current frame. Needed in order to retrieve
//                             the current swapchain and semaphore/fence values. It should
//                             be noted that this value is modified to represent the next frame
//                             upon completion of this function.
// @return true if the swapchain was out of date and a resize occured. False otherwise
void vulkan_core::EndFrame(u32             current_image_index,
                           VkCommandBuffer *command_buffers, /* used for the wait stage - buffer we wait for to complete */
                           u32             command_buffer_count)
{
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {
        SyncObjects.ImageAvailable[SyncObjects.CurrentFrame]
    };
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = command_buffer_count;
    submitInfo.pCommandBuffers = command_buffers;
    
    VkSemaphore signalSemaphores[] = {
        SyncObjects.RenderFinished[SyncObjects.CurrentFrame]
    };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    vk::vkResetFences(Device, 1,
                      &SyncObjects.InFlightFences[SyncObjects.CurrentFrame]);
    
    VkResult result = vk::vkQueueSubmit(GraphicsQueue.Handle, 1, &submitInfo,
                                        SyncObjects.InFlightFences[SyncObjects.CurrentFrame]);
    if (result != VK_SUCCESS)
    {
        mprinte("Failed to submit draw command buffer!");
        PlatformFatalError("Error while submitting the queue at end frame!\n");
    }
    
    //VK_CHECK_RESULT(vkQueueSubmit(GlobalVulkanState.GraphicsQueue.Handle, 1, &submitInfo,
    //GlobalVulkanState.SyncObjects.InFlightFences[GlobalVulkanState.SyncObjects.CurrentFrame]),
    //"Failed to submit draw command buffer!");
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {SwapChain.Handle};
    presentInfo.swapchainCount  = 1;
    presentInfo.pSwapchains     = swapChains;
    presentInfo.pImageIndices   = &current_image_index;
    presentInfo.pResults        = nullptr; // Optional
    
    vk::vkQueuePresentKHR(PresentQueue.Handle, &presentInfo);
    
    SyncObjects.CurrentFrame = (SyncObjects.CurrentFrame + 1) % sync_object_parameters::MAX_FRAMES;
}

void vulkan_core::SetViewport(VkCommandBuffer command_buffer,
                              u32             first_viewport,
                              u32             viewport_count,
                              VkViewport      *viewports)
{
    vk::vkCmdSetViewport(command_buffer, first_viewport, viewport_count, viewports);
}

void vulkan_core::SetScissor(VkCommandBuffer command_buffer,
                             u32             first_scissor,
                             u32             scissor_count,
                             VkRect2D        *scissors)
{
    vk::vkCmdSetScissor(command_buffer, first_scissor, scissor_count, scissors);
}


void vulkan_core::CreateCommandBuffers(VkCommandPool        command_pool,
                                       VkCommandBufferLevel level,
                                       uint32_t             command_buffer_count,
                                       VkCommandBuffer      *buffers)
{
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext              = NULL;
    alloc_info.commandPool        = command_pool;
    alloc_info.level              = level;
    alloc_info.commandBufferCount = command_buffer_count;
    
    VK_CHECK_RESULT(vk::vkAllocateCommandBuffers(Device, &alloc_info, buffers),
                    "Failed to allocate command buffers!");
}

void vulkan_core::DestroyCommandBuffers(VkCommandPool   command_pool,
                                        u32             command_buffer_count,
                                        VkCommandBuffer *buffers)
{
    vk::vkFreeCommandBuffers(Device, command_pool, command_buffer_count, buffers);
}

void vulkan_core::BeginCommandBuffer(VkCommandBuffer command_buffer) 
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    VK_CHECK_RESULT(vk::vkBeginCommandBuffer(command_buffer, &beginInfo),
                    "Failed to begin recording command buffer!");
}

void vulkan_core::EndCommandBuffer(VkCommandBuffer command_buffer) 
{
    VK_CHECK_RESULT(vk::vkEndCommandBuffer(command_buffer),
                    "Failed to record command buffer!");
}


VkRenderPass vulkan_core::CreateRenderPass(VkAttachmentDescription *attachments, 
                                           u32                      attachments_count,
                                           VkSubpassDescription    *subpasses, 
                                           u32                      subpass_count,
                                           VkSubpassDependency     *dependencies, 
                                           u32                      dependency_count)
{
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments_count;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = subpass_count;
    renderPassInfo.pSubpasses = subpasses;
    renderPassInfo.dependencyCount = dependency_count;
    renderPassInfo.pDependencies = dependencies;
    
    VkRenderPass render_pass;
    VK_CHECK_RESULT(vk::vkCreateRenderPass(Device, &renderPassInfo, nullptr, &render_pass),
                    "Failed to create render pass!");
    
    return render_pass;
}


void vulkan_core::DestroyRenderPass(VkRenderPass render_pass) 
{
    vk::vkDestroyRenderPass(Device, render_pass, nullptr);
}


void vulkan_core::BeginRenderPass(VkCommandBuffer command_buffer,
                                  VkClearValue    *clear_values,
                                  u32             clear_values_count,
                                  VkFramebuffer   framebuffer,
                                  VkRenderPass    render_pass) 
{
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = render_pass;
    renderPassInfo.framebuffer       = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = SwapChain.Extent;
    renderPassInfo.clearValueCount   = clear_values_count;
    renderPassInfo.pClearValues      = clear_values;
    
    vk::vkCmdBeginRenderPass(command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void vulkan_core::EndRenderPass(VkCommandBuffer command_buffer) 
{
    vk::vkCmdEndRenderPass(command_buffer);
}

VkFramebuffer vulkan_core::CreateFramebuffer(VkImageView *image_views, 
                                             u32          image_views_count,
                                             u32          image_index, 
                                             VkRenderPass render_pass)
{
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = render_pass;
    framebufferInfo.attachmentCount = image_views_count;
    framebufferInfo.pAttachments    = image_views;
    framebufferInfo.width           = SwapChain.Extent.width;
    framebufferInfo.height          = SwapChain.Extent.height;
    framebufferInfo.layers          = 1;
    
    VkFramebuffer framebuffer;
    VK_CHECK_RESULT(vk::vkCreateFramebuffer(Device, &framebufferInfo, nullptr, &framebuffer),
                    "Failed to create framebuffer!");
    
    return framebuffer;
}

void vulkan_core::DestroyFramebuffer(VkFramebuffer framebuffer) 
{
    vk::vkDestroyFramebuffer(Device, framebuffer, nullptr);
}


VkCommandPool vulkan_core::CreateCommandPool(VkCommandPoolCreateFlags flags) 
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = GraphicsQueue.FamilyIndex;
    poolInfo.flags = flags;
    
    VkCommandPool command_pool;
    VK_CHECK_RESULT(vk::vkCreateCommandPool(Device, &poolInfo, nullptr, &command_pool),
                    "Failed to create command pool!");
    
    return command_pool;
}

void vulkan_core::DestroyCommandPool(VkCommandPool command_pool)
{
    vk::vkDestroyCommandPool(Device, command_pool, nullptr);
}


VkFormat vulkan_core::FindDepthFormat()
{
    // Ordered from most to least desirable
    VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    for (u32 i = 0; i < sizeof(candidates)/sizeof(candidates[0]); ++i) {
        VkFormat format = candidates[i];
        VkFormatProperties props;
        vk::vkGetPhysicalDeviceFormatProperties(PhysicalDevice, format, &props);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            depthFormat = format;
            break;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            depthFormat = format;
            break;
        }
    }
    
    return depthFormat;
}

void vulkan_core::CreateVmaImage(VkImageCreateInfo       image_create_info,
                                 VmaAllocationCreateInfo vma_create_info,
                                 VkImage                 &image,
                                 VmaAllocation           &allocation,
                                 VmaAllocationInfo       &allocation_info)
{
    vmaCreateImage(VulkanAllocator,
                   &image_create_info,
                   &vma_create_info,
                   &image,
                   &allocation,
                   &allocation_info);
}

void vulkan_core::DestroyVmaImage(VkImage       image,
                                  VmaAllocation allocation)
{
    vmaDestroyImage(VulkanAllocator,
                    image, allocation);
}

void vulkan_core::TransitionImageLayout(VkCommandPool command_pool,
                                        VkImage image, VkFormat format,
                                        VkImageLayout oldLayout, VkImageLayout newLayout,
                                        u32 mip_levels)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(command_pool);
    
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = mip_levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.srcAccessMask                   = 0; // TODO
    barrier.dstAccessMask                   = 0; // TODO
    
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
            format == VK_FORMAT_D24_UNORM_S8_UINT) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } 
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } 
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } 
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else
    {
        // NOTE(Dustin): Silent failure
        mprinte("Unsupported layout transition!");
    }
    
    vk::vkCmdPipelineBarrier(commandBuffer,
                             sourceStage, destinationStage,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
    
    EndSingleTimeCommands(commandBuffer, command_pool);
}

void vulkan_core::CopyBufferToImage(VkCommandPool command_pool,
                                    VkBuffer buffer,
                                    VkImage image,
                                    u32 width, u32 height) 
{
    VkCommandBuffer command_buffer = BeginSingleTimeCommands(command_pool);
    
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    
    region.imageOffset = {0,0,0};
    region.imageExtent = {
        width,
        height,
        1
    };
    
    vk::vkCmdCopyBufferToImage(command_buffer,
                               buffer,
                               image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &region);
    
    EndSingleTimeCommands(command_buffer, command_pool);
}

VkCommandBuffer vulkan_core::BeginSingleTimeCommands(VkCommandPool command_pool)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = command_pool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vk::vkAllocateCommandBuffers(Device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vk::vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    return commandBuffer;
}

void vulkan_core::EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool command_pool)
{
    vk::vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vk::vkQueueSubmit(GraphicsQueue.Handle, 1, &submitInfo, VK_NULL_HANDLE);
    vk::vkQueueWaitIdle(GraphicsQueue.Handle);
    
    vk::vkFreeCommandBuffers(Device, command_pool, 1, &commandBuffer);
}


void vulkan_core::Idle() 
{
    vk::vkDeviceWaitIdle(Device);
}

void vulkan_core::CreateVmaBuffer(VkBufferCreateInfo      buffer_create_info,
                                  VmaAllocationCreateInfo vma_create_info,
                                  VkBuffer                &buffer,
                                  VmaAllocation           &allocation,
                                  VmaAllocationInfo       &allocation_info) 
{
    vmaCreateBuffer(VulkanAllocator,
                    &buffer_create_info,
                    &vma_create_info,
                    &buffer,
                    &allocation,
                    &allocation_info);
}

// Uploads the buffer to the GPU via a staging buffer
void vulkan_core::CreateVmaBufferWithStaging(VkBufferCreateInfo      buffer_create_info,
                                             VmaAllocationCreateInfo vma_create_info,
                                             VkCommandPool           CommandPool,
                                             VkBuffer                &buffer,
                                             VmaAllocation           &allocation,
                                             void                    *data,
                                             VkDeviceSize            size) 
{
    // Only create a staging buffer if the passed size is greater than zero
    // otherwise, just create the source buffer
    if (size > 0)
    {
        VkBuffer staging_buffer;
        VmaAllocation staging_allocation;
        
        VkBufferCreateInfo staging_buffer_info = {};
        staging_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        staging_buffer_info.size = size;
        staging_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        
        VmaAllocationCreateInfo alloc_info = {};
        alloc_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        
        VmaAllocationInfo info = {};
        CreateVmaBuffer(staging_buffer_info, alloc_info,
                        staging_buffer, staging_allocation, info);
        
        void *mapped_memory;
        VmaMap(&mapped_memory, staging_allocation);
        {
            memcpy(mapped_memory, data, size);
        }
        VmaUnmap(staging_allocation);
        
        CreateVmaBuffer(buffer_create_info, vma_create_info,
                        buffer, allocation, info);
        
        CopyBuffer(CommandPool, staging_buffer, buffer, size);
        
        DestroyVmaBuffer(staging_buffer, staging_allocation);
    }
    else
    {
        VmaAllocationInfo info = {};
        CreateVmaBuffer(buffer_create_info, vma_create_info,
                        buffer, allocation, info);
    }
}

void vulkan_core::DestroyVmaBuffer(VkBuffer buffer, VmaAllocation allocation)
{
    vmaDestroyBuffer(VulkanAllocator, buffer, allocation);
}


void vulkan_core::CopyBuffer(VkCommandPool command_pool,
                             VkBuffer      src_buffer,
                             VkBuffer      dst_buffer,
                             VkDeviceSize  size) 
{
    VkCommandBuffer command_buffer = BeginSingleTimeCommands(command_pool);
    
    VkBufferCopy copy_region = {};
    copy_region.size = size;
    vk::vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
    
    EndSingleTimeCommands(command_buffer, command_pool);
}

void vulkan_core::VmaMap(void **mapped_memory, VmaAllocation allocation) 
{
    vmaMapMemory(VulkanAllocator, allocation, mapped_memory);
}

void vulkan_core::VmaUnmap(VmaAllocation allocation)
{
    vmaUnmapMemory(VulkanAllocator, allocation);
}

void vulkan_core::VmaFlushAllocation(VmaAllocation Allocation, VkDeviceSize Offset, VkDeviceSize Size)
{
    vmaFlushAllocation(VulkanAllocator, Allocation, Offset, Size);
}

void vulkan_core::Map(void             **mapped_memory,
                      VkDeviceMemory   allocation,
                      VkDeviceSize     offset,
                      VkDeviceSize     size,
                      VkMemoryMapFlags flags)
{
    VK_CHECK_RESULT(vk::vkMapMemory(Device,
                                    allocation, offset, size, flags, mapped_memory),
                    "Unable to map buffer memory!");
}

void vulkan_core::Unmap(VkDeviceMemory allocation)
{
    vk::vkUnmapMemory(Device, allocation);
}


void vulkan_core::BindVertexBuffers(VkCommandBuffer command_buffer,
                                    u32             first_binding,
                                    u32             binding_count,
                                    VkBuffer        *buffers,
                                    VkDeviceSize    *offsets) 
{
    vk::vkCmdBindVertexBuffers(command_buffer, first_binding, binding_count, buffers, offsets);
}

void vulkan_core::BindIndexBuffer(VkCommandBuffer command_buffer,
                                  VkBuffer        buffer,
                                  VkDeviceSize    offset,
                                  VkIndexType     index_type) 
{
    vk::vkCmdBindIndexBuffer(command_buffer, buffer, offset, index_type);
}


VkShaderModule vulkan_core::CreateShaderModule(const u32 *code, size_t size) 
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode    = code;
    
    VkShaderModule shaderModule;
    VK_CHECK_RESULT(vk::vkCreateShaderModule(Device, &createInfo, nullptr, &shaderModule),
                    "Failed to create Shader module!\n");
    
    return shaderModule;
}

void vulkan_core::DestroyShaderModule(VkShaderModule module) 
{
    vk::vkDestroyShaderModule(Device, module, nullptr);
}


void vulkan_core::CreatePipelineCache(VkPipelineCache *PipelineCache)
{
    VkPipelineCacheCreateInfo PipelineCacheCreateInfo = {};
    PipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    
    VK_CHECK_RESULT(vk::vkCreatePipelineCache(Device, &PipelineCacheCreateInfo, nullptr, PipelineCache),
                    "Unable to create a pipeline cache!\n");
}

void vulkan_core::DestroyPipelineCache(VkPipelineCache PipelineCache)
{
    vk::vkDestroyPipelineCache(Device, PipelineCache, nullptr);
}

VkPipeline vulkan_core::CreatePipeline(VkGraphicsPipelineCreateInfo pipeline_info)
{
    return CreatePipeline(pipeline_info, VK_NULL_HANDLE);
}

VkPipeline vulkan_core::CreatePipeline(VkGraphicsPipelineCreateInfo pipeline_info, VkPipelineCache PipelineCache)
{
    VkPipeline pipeline;
    VK_CHECK_RESULT(vk::vkCreateGraphicsPipelines(Device,
                                                  PipelineCache, 1,
                                                  &pipeline_info, nullptr, &pipeline),
                    "Failed to create graphics pipeline!");
    return pipeline;
}


void vulkan_core::DestroyPipeline(VkPipeline pipeline)
{
    vk::vkDestroyPipeline(Device, pipeline, nullptr);
}

VkPipelineLayout vulkan_core::CreatePipelineLayout(VkPipelineLayoutCreateInfo layout_info) 
{
    VkPipelineLayout layout;
    VK_CHECK_RESULT(vk::vkCreatePipelineLayout(Device, &layout_info, nullptr, &layout),
                    "Failed to create pipeline layout!");
    
    return layout;
}

void vulkan_core::DestroyPipelineLayout(VkPipelineLayout pipeline_layout) 
{
    vk::vkDestroyPipelineLayout(Device, pipeline_layout, nullptr);
}

void vulkan_core::BindPipeline(VkCommandBuffer command_buffer, VkPipeline pipeline) 
{
    vk::vkCmdBindPipeline(command_buffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline);
}


void vulkan_core::Draw(VkCommandBuffer command_buffer,
                       u32             vertex_count,
                       u32             instance_count,
                       u32             first_vertex,
                       u32             first_instance) 
{
    vk::vkCmdDraw(command_buffer, vertex_count, instance_count, first_vertex, first_instance);
}

void vulkan_core::DrawIndexed(VkCommandBuffer command_buffer,
                              u32             index_count,
                              u32             instance_count,
                              u32             first_index,
                              u32             vertex_offset,
                              u32             first_instance) 
{
    vk::vkCmdDrawIndexed(command_buffer,
                         index_count,
                         instance_count,
                         first_index,
                         vertex_offset,
                         first_instance);
}

VkDescriptorSetLayout vulkan_core::CreateDescriptorSetLayout(VkDescriptorSetLayoutBinding *bindings, u32 bindings_count) 
{
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings_count;
    layoutInfo.pBindings = bindings;
    
    VkDescriptorSetLayout layout;
    VK_CHECK_RESULT(vk::vkCreateDescriptorSetLayout(Device,
                                                    &layoutInfo,
                                                    nullptr,
                                                    &layout),
                    "Failed to create descriptor set layout!");
    
    return layout;
}

void vulkan_core::DestroyDescriptorSetLayout(VkDescriptorSetLayout layout) 
{
    vk::vkDestroyDescriptorSetLayout(Device, layout, nullptr);
}


VkDescriptorPool vulkan_core::CreateDescriptorPool(VkDescriptorPoolSize *pool_sizes,
                                                   u32 pool_size_count,
                                                   u32 max_sets,
                                                   VkDescriptorPoolCreateFlags flags) 
{
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = pool_size_count;
    poolInfo.pPoolSizes    = pool_sizes;
    poolInfo.maxSets       = max_sets;
    poolInfo.flags         = flags;
    
    VkDescriptorPool descriptor_pool;
    VK_CHECK_RESULT(vk::vkCreateDescriptorPool(Device, &poolInfo, nullptr, &descriptor_pool),
                    "Failed to create descriptor pool!");
    
    return descriptor_pool;
}

void vulkan_core::DestroyDescriptorPool(VkDescriptorPool descriptor_pool) 
{
    vk::vkDestroyDescriptorPool(Device, descriptor_pool, nullptr);
}

void vulkan_core::ResetDescriptorPool(VkDescriptorPool descriptor_pool)
{
    VkDescriptorPoolResetFlags flags = {};
    
    vk::vkResetDescriptorPool(Device, descriptor_pool, flags);
}


void vulkan_core::CreateDescriptorSets(VkDescriptorSet *descriptor_sets,
                                       VkDescriptorSetAllocateInfo allocInfo) 
{
    VK_CHECK_RESULT(vk::vkAllocateDescriptorSets(Device,
                                                 &allocInfo,
                                                 descriptor_sets),
                    "Failed to create descriptor sets!");
}

void vulkan_core::DestroyDescriptorSets(VkDescriptorPool descriptor_pool,
                                        VkDescriptorSet *descriptor_sets,
                                        u32 descriptor_count) 
{
    vk::vkFreeDescriptorSets(Device,
                             descriptor_pool,
                             descriptor_count,
                             descriptor_sets);
}

void vulkan_core::UpdateDescriptorSets(VkWriteDescriptorSet *descriptor_set_writes,
                                       u32 write_count,
                                       u32 copy_count,
                                       VkCopyDescriptorSet* pstop) 
{
    vk::vkUpdateDescriptorSets(Device,
                               write_count,
                               descriptor_set_writes,
                               copy_count,
                               pstop);
}

void vulkan_core::BindDescriptorSets(VkCommandBuffer  command_buffer,
                                     VkPipelineLayout layout,
                                     u32              first_set,
                                     u32              descriptor_set_count,
                                     VkDescriptorSet  *descriptor_sets,
                                     u32              dynamic_offset_count,
                                     u32              *dynamic_offsets) 
{
    vk::vkCmdBindDescriptorSets(command_buffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                layout,
                                first_set,
                                descriptor_set_count,
                                descriptor_sets,
                                dynamic_offset_count,
                                dynamic_offsets);
}

u64 vulkan_core::GetMinUniformMemoryOffsetAlignment() 
{
    VkPhysicalDeviceProperties properties;
    vk::vkGetPhysicalDeviceProperties(PhysicalDevice, &properties);
    
    return properties.limits.minUniformBufferOffsetAlignment;
}

void vulkan_core::PushConstants(VkCommandBuffer    CommandBuffer,
                                VkPipelineLayout   Layout,
                                VkShaderStageFlags StageFlags,
                                u32                Offset,
                                u32                Size,
                                const void*        pValues)
{
    vk::vkCmdPushConstants(CommandBuffer, Layout, StageFlags, Offset, Size, pValues);
}
