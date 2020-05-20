
namespace vk {
    
    VulkanCore GlobalVulkanState = {};
    
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
                                                DynamicArray<char const *> &enabled_extensions)
{
#define VK_INSTANCE_LEVEL_FUNCTION(fun)                                \
    if (!(fun = (PFN_##fun)vkGetInstanceProcAddr(instance, #fun))) {   \
                 mprinte("Could not load instance level function: %s!\n", #fun); \
                         return false;                                                  \
}

#define VK_INSTANCE_LEVEL_FUNCTION_FROM_EXTENSION(fun, extension)       \
for (u32 i = 0; i < enabled_extensions.Size(); ++i) {               \
    if (std::string(enabled_extensions[i]) == std::string(extension)) { \
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
                                              DynamicArray<char const *> &enabled_extensions)
{
#define VK_DEVICE_LEVEL_FUNCTION(fun)                                    \
    if (!(fun = (PFN_##fun)vkGetDeviceProcAddr(logical_device, #fun))) { \
                 mprinte("Could not load device level function: %s!\n", #fun);    \
                         return false;                                                   \
}

#define VK_DEVICE_LEVEL_FUNCTION_FROM_EXTENSION(fun, extension)         \
for (u32 i = 0; i < enabled_extensions.Size(); ++i) {               \
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

bool CheckValidationLayerSupport()
{
    u32 layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    VkLayerProperties *availableLayers = (VkLayerProperties*)talloc(sizeof(VkLayerProperties) * layerCount);
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
    
    return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData)
{
    
    PlatformPrintMessage(EConsoleColor::Yellow, EConsoleColor::DarkGrey, "%s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

// TODO(Dustin): Load this through the loader like all other functions
VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(GlobalVulkanState.Instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(GlobalVulkanState.Instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// TODO(Dustin): Load this through the loader like all other functions
void DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(GlobalVulkanState.Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(GlobalVulkanState.Instance, debugMessenger, pAllocator);
    }
}


void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
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

void SetupDebugMessenger()
{
    if (!GlobalEnabledValidationLayers) return;
    
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    PopulateDebugMessengerCreateInfo(createInfo);
    
    VK_CHECK_RESULT(CreateDebugUtilsMessengerEXT(&createInfo, nullptr, &GlobalDebugMessenger),
                    "Failed to set up debug messenger!");
}

file_internal VkSampleCountFlagBits GetMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(GlobalVulkanState.PhysicalDevice, &physicalDeviceProperties);
    
    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
    
    return VK_SAMPLE_COUNT_1_BIT;
}

file_internal bool CreateInstance()
{
    if (GlobalEnabledValidationLayers && !CheckValidationLayerSupport())
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
    
    DynamicArray<const char*> exts = DynamicArray<const char*>();
    exts.Resize(3);
    
    const char *surface_exts = "VK_KHR_surface";
    const char* plat_exts = PlatformGetRequiredInstanceExtensions(GlobalEnabledValidationLayers);
    exts.PushBack(surface_exts);
    exts.PushBack(plat_exts);
    
    if (GlobalValidationLayers)
    {
        const char *name = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        exts.PushBack(name);
    }
    
    createInfo.enabledExtensionCount   = (u32)exts.Size();
    createInfo.ppEnabledExtensionNames = exts.GetArray();
    
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (GlobalEnabledValidationLayers) {
        createInfo.enabledLayerCount   = GlobalValidationCount;
        createInfo.ppEnabledLayerNames = GlobalValidationLayers;
        
        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        
        createInfo.pNext = nullptr;
    }
    
    VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &GlobalVulkanState.Instance),
                    "Failed to create instance!");
    
    assert(GlobalVulkanState.Instance != NULL);
    
    return true;
}

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physical_device)
{
    QueueFamilyIndices indices;
    
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyCount, nullptr);
    
    VkQueueFamilyProperties *queueFamilies =
        (VkQueueFamilyProperties*)talloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyCount, queueFamilies);
    
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
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i,
                                             GlobalVulkanState.PresentationSurface,
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
    
    return indices;
}

file_internal bool CheckDeviceExtensionSupport(VkPhysicalDevice physical_device)
{
    u32 extensionCount;
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, nullptr);
    
    VkExtensionProperties *availableExtensions = talloc<VkExtensionProperties>(extensionCount);
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, availableExtensions);
    
    // insert extensions into a set to make sure they are all unique
    std::set<std::string> requiredExtensions;
    for (u32 i = 0; i < GlobalDeviceExtensionsCount; ++i)
        requiredExtensions.insert(GlobalDeviceExtensions[i]);
    
    for (u32 i = 0; i < extensionCount; ++i)
    {
        VkExtensionProperties extension = availableExtensions[i];
        requiredExtensions.erase(extension.extensionName);
    }
    
    return requiredExtensions.empty();
}

SwapChainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physical_device)
{
    SwapChainSupportDetails details;
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device,
                                              GlobalVulkanState.PresentationSurface,
                                              &details.Capabilities);
    
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device,
                                         GlobalVulkanState.PresentationSurface,
                                         &details.FormatsCount,
                                         nullptr);
    
    if (details.FormatsCount != 0)
    {
        details.Formats = talloc<VkSurfaceFormatKHR>(details.FormatsCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device,
                                             GlobalVulkanState.PresentationSurface,
                                             &details.FormatsCount,
                                             details.Formats);
    }
    
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device,
                                              GlobalVulkanState.PresentationSurface,
                                              &details.PresentModesCount,
                                              nullptr);
    
    if (details.PresentModesCount != 0)
    {
        details.PresentModes = talloc<VkPresentModeKHR>(details.PresentModesCount);
        
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device,
                                                  GlobalVulkanState.PresentationSurface,
                                                  &details.PresentModesCount,
                                                  details.PresentModes);
    }
    
    return details;
}

file_internal bool IsDeviceSuitable(VkPhysicalDevice physical_device)
{
    QueueFamilyIndices indices = FindQueueFamilies(physical_device);
    if (!indices.isComplete()) return false;
    
    bool extensionsSupported = CheckDeviceExtensionSupport(physical_device);
    if (!extensionsSupported) return false;
    
    // Query Swapchain support
    SwapChainSupportDetails details = QuerySwapchainSupport(physical_device);
    
    if (details.FormatsCount != 0 && details.PresentModesCount != 0)
        return true;
    
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physical_device, &supportedFeatures);
    return (supportedFeatures.samplerAnisotropy);
}

file_internal void PickPhysicalDevice(VkInstance instance)
{
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    
    if (deviceCount == 0)
    {
        mprinte("Failed to find a GPU with Vulkan support!\n");
        return;
    }
    
    VkPhysicalDevice *devices = talloc<VkPhysicalDevice>(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
    
    // Finds the first suitable device
    for (u32 i = 0; i < deviceCount; ++i)
    {
        VkPhysicalDevice device = devices[i];
        
        if (IsDeviceSuitable(device))
        {
            GlobalVulkanState.PhysicalDevice = device;
            GlobalVulkanState.MsaaSamples = GetMaxUsableSampleCount();
            break;
        }
    }
    
    if (GlobalVulkanState.PhysicalDevice == VK_NULL_HANDLE)
    {
        mprinte("Failed to find a suitable GPU!\n");
        return;
    }
}

void CreateLogicalDevice()
{
    QueueFamilyIndices indices = FindQueueFamilies(GlobalVulkanState.PhysicalDevice);
    
    // TODO(Dustin): Remove set
    std::set<u32> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };
    
    VkDeviceQueueCreateInfo *queueCreateInfos = talloc<VkDeviceQueueCreateInfo>(uniqueQueueFamilies.size());
    
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
    
    VK_CHECK_RESULT(vkCreateDevice(GlobalVulkanState.PhysicalDevice,
                                   &createInfo, nullptr,
                                   &GlobalVulkanState.Device),
                    "Failed to logical device!");
    
    
}

file_internal u32 FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(GlobalVulkanState.PhysicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    // NOTE(Dustin): Silent failure. Watch out!
    mprinte("Failed to find suitable memory type!\n");
    return 0;
}

file_internal void CreateSwapchain(SwapChainParameters &swapchain_params)
{
    // Query Swapchain support
    SwapChainSupportDetails details = QuerySwapchainSupport(GlobalVulkanState.PhysicalDevice);
    
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
    createInfo.surface = GlobalVulkanState.PresentationSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surface_format.format;
    createInfo.imageColorSpace = surface_format.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    u32 queueFamilyIndices[] = {
        GlobalVulkanState.GraphicsQueue.FamilyIndex,
        GlobalVulkanState.PresentQueue.FamilyIndex,
    };
    
    if (GlobalVulkanState.GraphicsQueue.FamilyIndex != GlobalVulkanState.PresentQueue.FamilyIndex)
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
    
    VK_CHECK_RESULT(vkCreateSwapchainKHR(GlobalVulkanState.Device,
                                         &createInfo, nullptr,
                                         &swapchain_params.Handle),
                    "Failed to create the swap chain!");
    
    vkGetSwapchainImagesKHR(GlobalVulkanState.Device, swapchain_params.Handle, &imageCount, nullptr);
    
    swapchain_params.ImagesCount = (imageCount);
    swapchain_params.Images = palloc<ImageParameters>(imageCount);
    
    // @Cleanup: In order to get the swapchain images, need to alloc a temp
    // array to retrieve the images
    VkImage *images = talloc<VkImage>(imageCount);
    vkGetSwapchainImagesKHR(GlobalVulkanState.Device, swapchain_params.Handle, &imageCount, images);
    
    for (u32 i = 0; i < imageCount; ++i)
    {
        ImageParameters iparam = {};
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
}

file_internal void CreateSyncObjects(SyncObjectParameters &sync_objects)
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (int i = 0; i < SyncObjectParameters::MAX_FRAMES; ++i)
    {
        VK_CHECK_RESULT(vkCreateSemaphore(GlobalVulkanState.Device, &semaphoreInfo, nullptr, &sync_objects.ImageAvailable[i]),
                        "Failed to create synchronization objects for a frame!");
        VK_CHECK_RESULT(vkCreateSemaphore(GlobalVulkanState.Device, &semaphoreInfo, nullptr, &sync_objects.RenderFinished[i]),
                        "Failed to create synchronization objects for a frame!");
        VK_CHECK_RESULT(vkCreateFence(GlobalVulkanState.Device, &fenceInfo, nullptr, &sync_objects.InFlightFences[i]),
                        "Failed to create synchronization objects for a frame!");
    }
}

VkCommandBuffer BeginSingleTimeCommands(VkCommandPool command_pool)
{
    
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = command_pool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(GlobalVulkanState.Device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    return commandBuffer;
}

void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool command_pool)
{
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(GlobalVulkanState.GraphicsQueue.Handle, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(GlobalVulkanState.GraphicsQueue.Handle);
    
    vkFreeCommandBuffers(GlobalVulkanState.Device, command_pool, 1, &commandBuffer);
}
}

bool vk::InitializeVulkan()
{
    // load the Vulkan Library
    if (!LoadVulkanLibrary())
        return false;
    
    // Load export function
    if (!LoadExportedEntryPoints())
        return false;
    
    // Load Global Entry functions
    if (!LoadGlobalLevelEntryPoints())
        return false;
    
    if (!CreateInstance())
        return false;
    SetupDebugMessenger();
    
    // Load Instance Level Functions
    DynamicArray<const char *> instance_extensions(2);
    
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
    
    instance_extensions.PushBack(khr_surface_name);
    instance_extensions.PushBack(plat_surface);
    
    if (!LoadInstanceLevelEntryPoints(GlobalVulkanState.Instance, instance_extensions))
        return false;
    
    GlobalVulkanState.PresentationSurface = VK_NULL_HANDLE;
    PlatformVulkanCreateSurface(&GlobalVulkanState.PresentationSurface, GlobalVulkanState.Instance);
    assert(GlobalVulkanState.PresentationSurface != VK_NULL_HANDLE);
    
    PickPhysicalDevice(GlobalVulkanState.Instance);
    CreateLogicalDevice();
    
    // Load all Device related functions
    DynamicArray<const char *> device_extensions(1);
    
    const char *khr_swapchain_name = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    
    device_extensions.PushBack(khr_swapchain_name);
    
#if 1
    if (!LoadDeviceLevelEntryPoints(GlobalVulkanState.Device, device_extensions))
        return false;
#endif
    
    // Retrive the Graphics and Present Queue from the logical device
    {
        QueueFamilyIndices indices = FindQueueFamilies(GlobalVulkanState.PhysicalDevice);
        
        GlobalVulkanState.GraphicsQueue = {};
        GlobalVulkanState.PresentQueue = {};
        
        vkGetDeviceQueue(GlobalVulkanState.Device, indices.graphicsFamily.value(), 0,
                         &GlobalVulkanState.GraphicsQueue.Handle);
        vkGetDeviceQueue(GlobalVulkanState.Device, indices.presentFamily.value(), 0,
                         &GlobalVulkanState.PresentQueue.Handle);
        
        GlobalVulkanState.GraphicsQueue.FamilyIndex = indices.graphicsFamily.value();
        GlobalVulkanState.PresentQueue.FamilyIndex  = indices.presentFamily.value();
    }
    
    CreateSwapchain(GlobalVulkanState.SwapChain);
    CreateSyncObjects(GlobalVulkanState.SyncObjects);
    
    // Setup Vulkan Proxy Allocator
    //mm::Allocator *GlobalPermanantStorage = mm::GetPermanantStorage();
    //void *ptr = (void*)palloc<jengine::mm::VulkanProxyAllocator>(1);
    //GlobalVulkanState.VkProxyAllocator = new (ptr) jengine::mm::VulkanProxyAllocator(*GlobalPermanantStorage);
    
    //GlobalVulkanState.VkProxyAllocator = jengin::mm::VulkanProxyAllocator(*GlobalPermanantStorage);
    
    
    VmaAllocatorCreateInfo alloc_info = {};
    alloc_info.physicalDevice = GlobalVulkanState.PhysicalDevice;
    alloc_info.device = GlobalVulkanState.Device;
    alloc_info.instance = GlobalVulkanState.Instance;
    
    // Might have to heap allocate?
    VmaVulkanFunctions vmaf= {};
    vmaf.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vmaf.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vmaf.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    vmaf.vkAllocateMemory = vkAllocateMemory;
    vmaf.vkFreeMemory = vkFreeMemory;
    vmaf.vkMapMemory = vkMapMemory;
    vmaf.vkUnmapMemory = vkUnmapMemory;
    vmaf.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    vmaf.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    vmaf.vkBindBufferMemory = vkBindBufferMemory;
    vmaf.vkBindImageMemory = vkBindImageMemory;
    vmaf.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    vmaf.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    vmaf.vkCreateBuffer = vkCreateBuffer;
    vmaf.vkDestroyBuffer = vkDestroyBuffer;
    vmaf.vkCreateImage = vkCreateImage;
    vmaf.vkDestroyImage = vkDestroyImage;
    vmaf.vkCmdCopyBuffer = vkCmdCopyBuffer;
#if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
    //vmaf.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
    //vmaf.vkGetImageMemoryRequirements2KHR  = vkGetImageMemoryRequirements2KHR;
#endif
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
    //vmaf.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
    //vmaf.vkBindImageMemory2KHR  = vkBindImageMemory2KHR;
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
    //vmaf.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
#endif
    
    alloc_info.pVulkanFunctions = &vmaf;
    //alloc_info.pAllocationCallbacks = &GlobalVulkanState.VkProxyAllocator->GetVkAllocationCallbacks();
    
    vmaCreateAllocator(&alloc_info, &GlobalVulkanState.VulkanAllocator);
    
    return true;
}

void vk::ShutdownVulkan()
{
    vmaDestroyAllocator(GlobalVulkanState.VulkanAllocator);
    
    for (int i = 0; i < SyncObjectParameters::MAX_FRAMES; ++i)
    {
        vkDestroySemaphore(GlobalVulkanState.Device, GlobalVulkanState.SyncObjects.RenderFinished[i], nullptr);
        vkDestroySemaphore(GlobalVulkanState.Device, GlobalVulkanState.SyncObjects.ImageAvailable[i], nullptr);
        vkDestroyFence(GlobalVulkanState.Device,     GlobalVulkanState.SyncObjects.InFlightFences[i], nullptr);
    }
    
    // Free the swapchain
    for (u32 i = 0; i < GlobalVulkanState.SwapChain.ImagesCount; ++i) {
        ImageParameters iparam = GlobalVulkanState.SwapChain.Images[i];
        vkDestroyImageView(GlobalVulkanState.Device, iparam.View, nullptr);
    }
    //GlobalVulkanState.SwapChain.Images.Resize(0);
    pfree(GlobalVulkanState.SwapChain.Images);
    GlobalVulkanState.SwapChain.ImagesCount = 0;
    
    vkDestroySwapchainKHR(GlobalVulkanState.Device, GlobalVulkanState.SwapChain.Handle, nullptr);
    
    vkDestroyDevice(GlobalVulkanState.Device, nullptr);
    if (GlobalEnabledValidationLayers) {
        DestroyDebugUtilsMessengerEXT(GlobalDebugMessenger, nullptr);
    }
    
    vkDestroySurfaceKHR(GlobalVulkanState.Instance, GlobalVulkanState.PresentationSurface, nullptr);
    vkDestroyInstance(GlobalVulkanState.Instance, nullptr);
    
    //pfree(GlobalVulkanState.VkProxyAllocator);
}

void vk::Idle() {
    vkDeviceWaitIdle(GlobalVulkanState.Device);
}

VkImageView vk::CreateImageView(VkImageViewCreateInfo create_info)
{
    VkImageView image_view;
    VK_CHECK_RESULT(vkCreateImageView(GlobalVulkanState.Device, &create_info, nullptr, &image_view),
                    "Failed to create texture image view!");
    
    return image_view;
}

void vk::DestroyImageView(VkImageView image_view) {
    vkDestroyImageView(GlobalVulkanState.Device, image_view, nullptr);
}

VkSampler vk::CreateImageSampler(VkSamplerCreateInfo sampler_info) {
    VkSampler sampler;
    VK_CHECK_RESULT(vkCreateSampler(GlobalVulkanState.Device, &sampler_info, nullptr, &sampler),
                    "Could create image sampler!");
    
    return sampler;
}

void vk::DestroyImageSampler(VkSampler sampler) {
    vkDestroySampler(GlobalVulkanState.Device, sampler, nullptr);
}

void vk::DestroyImage(VkImage        image,
                      VkDeviceMemory allocation)
{
    vkDestroyImage(GlobalVulkanState.Device,
                   image,
                   nullptr);
}

void vk::DestroyVmaImage(VkImage       image,
                         VmaAllocation allocation)
{
    vmaDestroyImage(GlobalVulkanState.VulkanAllocator,
                    image, allocation);
}

void vk::CreateImage(VkImageCreateInfo       image_create_info,
                     VmaAllocationCreateInfo vma_create_info,
                     VkImage                 &image,
                     VkDeviceMemory          &allocation)
{
    VK_CHECK_RESULT(vkCreateImage(GlobalVulkanState.Device,
                                  &image_create_info,
                                  nullptr,
                                  &image),
                    "Failed to create image!\n");
}

void vk::CreateVmaImage(VkImageCreateInfo       image_create_info,
                        VmaAllocationCreateInfo vma_create_info,
                        VkImage                 &image,
                        VmaAllocation           &allocation,
                        VmaAllocationInfo       &allocation_info)
{
    vmaCreateImage(GlobalVulkanState.VulkanAllocator,
                   &image_create_info,
                   &vma_create_info,
                   &image,
                   &allocation,
                   &allocation_info);
}

void vk::TransitionImageLayout(VkCommandPool command_pool,
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
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        // NOTE(Dustin): Silent failure
        mprinte("Unsupported layout transition!");
    }
    
    vkCmdPipelineBarrier(commandBuffer,
                         sourceStage, destinationStage,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier
                         );
    
    EndSingleTimeCommands(commandBuffer, command_pool);
}


VkRenderPass vk::CreateRenderPass(VkAttachmentDescription *attachments, u32 attachments_count,
                                  VkSubpassDescription *subpasses, u32 subpass_count,
                                  VkSubpassDependency *dependencies, u32 dependency_count)
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
    VK_CHECK_RESULT(vkCreateRenderPass(GlobalVulkanState.Device, &renderPassInfo, nullptr, &render_pass),
                    "Failed to create render pass!");
    
    return render_pass;
}

ImageParameters* vk::GetSwapChainImages()
{
    return GlobalVulkanState.SwapChain.Images;
}

u32 vk::GetSwapChainImageCount() {
    return GlobalVulkanState.SwapChain.ImagesCount;
}


VkFormat vk::GetSwapChainImageFormat() {
    return GlobalVulkanState.SwapChain.Format;
}

VkExtent2D vk::GetSwapChainExtent() {
    return GlobalVulkanState.SwapChain.Extent;
}

VkFormat vk::FindDepthFormat()
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
        vkGetPhysicalDeviceFormatProperties(GlobalVulkanState.PhysicalDevice, format, &props);
        
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

void vk::DestroyRenderPass(VkRenderPass render_pass) {
    vkDestroyRenderPass(GlobalVulkanState.Device, render_pass, nullptr);
}

VkCommandPool vk::CreateCommandPool(VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = GlobalVulkanState.GraphicsQueue.FamilyIndex;
    poolInfo.flags = flags;
    
    VkCommandPool command_pool;
    VK_CHECK_RESULT(vkCreateCommandPool(GlobalVulkanState.Device, &poolInfo, nullptr, &command_pool),
                    "Failed to create command pool!");
    
    return command_pool;
}

void vk::DestroyCommandPool(VkCommandPool command_pool) {
    vkDestroyCommandPool(GlobalVulkanState.Device, command_pool, nullptr);
}

VkFramebuffer vk::CreateFramebuffer(VkImageView *image_views, u32 image_views_count,
                                    u32 image_index, VkRenderPass render_pass)
{
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = render_pass;
    framebufferInfo.attachmentCount = image_views_count;
    framebufferInfo.pAttachments    = image_views;
    framebufferInfo.width           = GlobalVulkanState.SwapChain.Extent.width;
    framebufferInfo.height          = GlobalVulkanState.SwapChain.Extent.height;
    framebufferInfo.layers          = 1;
    
    VkFramebuffer framebuffer;
    VK_CHECK_RESULT(vkCreateFramebuffer(GlobalVulkanState.Device, &framebufferInfo, nullptr, &framebuffer),
                    "Failed to create framebuffer!");
    
    return framebuffer;
}

void vk::DestroyFramebuffer(VkFramebuffer framebuffer) {
    vkDestroyFramebuffer(GlobalVulkanState.Device, framebuffer, nullptr);
}

void vk::DestroyMemory(VkDeviceMemory memory) {
    vkFreeMemory(GlobalVulkanState.Device, memory, nullptr);
}

void vk::CreateCommandBuffers(VkCommandPool        command_pool,
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
    
    VK_CHECK_RESULT(vkAllocateCommandBuffers(GlobalVulkanState.Device, &alloc_info, buffers),
                    "Failed to allocate command buffers!");
}

void vk::DestroyCommandBuffers(VkCommandPool   command_pool,
                               u32             command_buffer_count,
                               VkCommandBuffer *buffers)
{
    vkFreeCommandBuffers(GlobalVulkanState.Device, command_pool, command_buffer_count, buffers);
}


// @return false if the swapchain was out of date and a resize occured. true otherwise
VkResult vk::BeginFrame(u32          &next_image_idx) {
    vkWaitForFences(GlobalVulkanState.Device, 1,
                    &GlobalVulkanState.SyncObjects.InFlightFences[GlobalVulkanState.SyncObjects.CurrentFrame],
                    VK_TRUE, UINT64_MAX);
    
    // Draw frame
    VkResult khr_result = vkAcquireNextImageKHR(GlobalVulkanState.Device,
                                                GlobalVulkanState.SwapChain.Handle,
                                                UINT64_MAX,
                                                GlobalVulkanState.SyncObjects.ImageAvailable[GlobalVulkanState.SyncObjects.CurrentFrame],
                                                VK_NULL_HANDLE,
                                                &next_image_idx);
    
    return khr_result;
}

// @param current_frame_index, represents the current frame. Needed in order to retrieve
//                             the current swapchain and semaphore/fence values. It should
//                             be noted that this value is modified to represent the next frame
//                             upon completion of this function.
// @return true if the swapchain was out of date and a resize occured. False otherwise
void vk::EndFrame(u32             current_image_index,
                  VkCommandBuffer *command_buffers, /* used for the wait stage - buffer we wait for to complete */
                  u32             command_buffer_count)
{
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {
        GlobalVulkanState.SyncObjects.ImageAvailable[GlobalVulkanState.SyncObjects.CurrentFrame]
    };
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = command_buffer_count;
    submitInfo.pCommandBuffers = command_buffers;
    
    VkSemaphore signalSemaphores[] = {
        GlobalVulkanState.SyncObjects.RenderFinished[GlobalVulkanState.SyncObjects.CurrentFrame]
    };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    vkResetFences(GlobalVulkanState.Device, 1,
                  &GlobalVulkanState.SyncObjects.InFlightFences[GlobalVulkanState.SyncObjects.CurrentFrame]);
    
    VkResult result = vkQueueSubmit(GlobalVulkanState.GraphicsQueue.Handle, 1, &submitInfo,
                                    GlobalVulkanState.SyncObjects.InFlightFences[GlobalVulkanState.SyncObjects.CurrentFrame]);
    if (result != VK_SUCCESS)
    {
        mprinte("Failed to submit draw command buffer!");
        throw std::runtime_error("Error while submitting the queue at end frame!\n");
        
    }
    //VK_CHECK_RESULT(vkQueueSubmit(GlobalVulkanState.GraphicsQueue.Handle, 1, &submitInfo,
    //GlobalVulkanState.SyncObjects.InFlightFences[GlobalVulkanState.SyncObjects.CurrentFrame]),
    //"Failed to submit draw command buffer!");
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {GlobalVulkanState.SwapChain.Handle};
    presentInfo.swapchainCount  = 1;
    presentInfo.pSwapchains     = swapChains;
    presentInfo.pImageIndices   = &current_image_index;
    presentInfo.pResults        = nullptr; // Optional
    
    vkQueuePresentKHR(GlobalVulkanState.PresentQueue.Handle, &presentInfo);
    
    GlobalVulkanState.SyncObjects.CurrentFrame = (GlobalVulkanState.SyncObjects.CurrentFrame + 1) % SyncObjectParameters::MAX_FRAMES;
}

void vk::SetViewport(VkCommandBuffer command_buffer,
                     u32             first_viewport,
                     u32             viewport_count,
                     VkViewport      *viewports) {
    vkCmdSetViewport(command_buffer, first_viewport, viewport_count, viewports);
}

void vk::SetScissor(VkCommandBuffer command_buffer,
                    u32             first_scissor,
                    u32             scissor_count,
                    VkRect2D        *scissors) {
    vkCmdSetScissor(command_buffer, first_scissor, scissor_count, scissors);
}

void vk::BeginCommandBuffer(VkCommandBuffer command_buffer) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    VK_CHECK_RESULT(vkBeginCommandBuffer(command_buffer, &beginInfo),
                    "Failed to begin recording command buffer!");
}

void vk::EndCommandBuffer(VkCommandBuffer command_buffer) {
    VK_CHECK_RESULT(vkEndCommandBuffer(command_buffer),
                    "Failed to record command buffer!");
}

void vk::BeginRenderPass(VkCommandBuffer command_buffer,
                         VkClearValue    *clear_values,
                         u32             clear_values_count,
                         VkFramebuffer   framebuffer,
                         VkRenderPass    render_pass) {
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = render_pass;
    renderPassInfo.framebuffer       = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = GlobalVulkanState.SwapChain.Extent;
    renderPassInfo.clearValueCount   = clear_values_count;
    renderPassInfo.pClearValues      = clear_values;
    
    vkCmdBeginRenderPass(command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void vk::EndRenderPass(VkCommandBuffer command_buffer) {
    vkCmdEndRenderPass(command_buffer);
}

void vk::Resize() {
    // Free the swapchain
    for (u32 i = 0; i < GlobalVulkanState.SwapChain.ImagesCount; ++i) {
        ImageParameters iparam = GlobalVulkanState.SwapChain.Images[i];
        vkDestroyImageView(GlobalVulkanState.Device, iparam.View, nullptr);
    }
    pfree(GlobalVulkanState.SwapChain.Images);
    GlobalVulkanState.SwapChain.ImagesCount = 0;
    
    vkDestroySwapchainKHR(GlobalVulkanState.Device, GlobalVulkanState.SwapChain.Handle, nullptr);
    
    // Re-create the swapchain
    CreateSwapchain(GlobalVulkanState.SwapChain);
}

void vk::CreateBuffer(VkBufferCreateInfo      buffer_create_info,
                      VkMemoryAllocateInfo    allocation_create_info,
                      VkBuffer                &buffer,
                      VkDeviceMemory          &allocation) {
    VK_CHECK_RESULT(vkCreateBuffer(GlobalVulkanState.Device,
                                   &buffer_create_info,
                                   nullptr,
                                   &buffer),
                    "Failed to create a buffer!\n");
    
    VK_CHECK_RESULT(vkAllocateMemory(GlobalVulkanState.Device,
                                     &allocation_create_info,
                                     nullptr,
                                     &allocation),
                    "Failed to allocate memory for a buffer!");
}


void vk::CreateVmaBuffer(VkBufferCreateInfo      buffer_create_info,
                         VmaAllocationCreateInfo vma_create_info,
                         VkBuffer                &buffer,
                         VmaAllocation           &allocation,
                         VmaAllocationInfo       &allocation_info) {
    vmaCreateBuffer(GlobalVulkanState.VulkanAllocator,
                    &buffer_create_info,
                    &vma_create_info,
                    &buffer,
                    &allocation,
                    &allocation_info);
}

// Uploads the buffer to the GPU via a staging buffer
void vk::CreateVmaBufferWithStaging(VkCommandPool           command_pool,
                                    VkBufferCreateInfo      buffer_create_info,
                                    VmaAllocationCreateInfo vma_create_info,
                                    VkBuffer                &buffer,
                                    VmaAllocation           &allocation,
                                    void                    *data,
                                    VkDeviceSize            size) {
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
    
    
    CopyBuffer(command_pool, staging_buffer, buffer, size);
    
    DestroyVmaBuffer(staging_buffer, staging_allocation);
}

void  CreateBufferWithStaging(VkCommandPool           command_pool,
                              VkBufferCreateInfo      buffer_create_info,
                              VkMemoryAllocateInfo    memory_create_info,
                              VkBuffer                &buffer,
                              VkDeviceMemory          &allocation,
                              void                    *data,
                              VkDeviceSize            size)
{
}


void vk::DestroyVmaBuffer(VkBuffer buffer, VmaAllocation allocation) {
    vmaDestroyBuffer(GlobalVulkanState.VulkanAllocator, buffer, allocation);
}


void DestroyDeviceMemory(VkDeviceMemory device_memory)
{
}


void vk::DestroyBuffer(VkBuffer buffer, VkDeviceMemory allocation)
{
}

void vk::CopyBuffer(VkCommandPool command_pool,
                    VkBuffer      src_buffer,
                    VkBuffer      dst_buffer,
                    VkDeviceSize  size) {
    VkCommandBuffer command_buffer = BeginSingleTimeCommands(command_pool);
    
    VkBufferCopy copy_region = {};
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
    
    EndSingleTimeCommands(command_buffer, command_pool);
}

void vk::VmaMap(void **mapped_memory, VmaAllocation allocation) {
    vmaMapMemory(GlobalVulkanState.VulkanAllocator, allocation, mapped_memory);
}

void vk::VmaUnmap(VmaAllocation allocation) {
    vmaUnmapMemory(GlobalVulkanState.VulkanAllocator, allocation);
}


void vk::Map(void             **mapped_memory,
             VkDeviceMemory   allocation,
             VkDeviceSize     offset,
             VkDeviceSize     size,
             VkMemoryMapFlags flags)
{
}

void vk::Unmap(VkDeviceMemory allocation)
{
}


VkShaderModule vk::CreateShaderModule(const char *code, size_t size) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = (u32*)code;
    
    VkShaderModule shaderModule;
    VK_CHECK_RESULT(vkCreateShaderModule(GlobalVulkanState.Device, &createInfo, nullptr, &shaderModule),
                    "Failed to create Shader module!\n");
    
    return shaderModule;
}

void vk::DestroyShaderModule(VkShaderModule module) {
    vkDestroyShaderModule(GlobalVulkanState.Device, module, nullptr);
}

VkPipeline vk::CreatePipeline(VkGraphicsPipelineCreateInfo pipeline_info) {
    VkPipeline pipeline;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(GlobalVulkanState.Device,
                                              VK_NULL_HANDLE, 1,
                                              &pipeline_info, nullptr, &pipeline),
                    "Failed to create graphics pipeline!");
    
    return pipeline;
}

void vk::DestroyPipeline(VkPipeline pipeline) {
    vkDestroyPipeline(GlobalVulkanState.Device, pipeline, nullptr);
}

VkPipelineLayout vk::CreatePipelineLayout(VkPipelineLayoutCreateInfo layout_info) {
    VkPipelineLayout layout;
    VK_CHECK_RESULT(vkCreatePipelineLayout(GlobalVulkanState.Device, &layout_info, nullptr, &layout),
                    "Failed to create pipeline layout!");
    
    return layout;
}

void vk::DestroyPipelineLayout(VkPipelineLayout pipeline_layout) {
    vkDestroyPipelineLayout(GlobalVulkanState.Device, pipeline_layout, nullptr);
}

void vk::BindPipeline(VkCommandBuffer command_buffer, VkPipeline pipeline) {
    vkCmdBindPipeline(command_buffer,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipeline);
}

VkDescriptorSetLayout vk::CreateDescriptorSetLayout(VkDescriptorSetLayoutBinding *bindings, u32 bindings_count) {
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings_count;
    layoutInfo.pBindings = bindings;
    
    VkDescriptorSetLayout layout;
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(GlobalVulkanState.Device,
                                                &layoutInfo,
                                                nullptr,
                                                &layout),
                    "Failed to create descriptor set layout!");
    
    return layout;
}

void vk::DestroyDescriptorSetLayout(VkDescriptorSetLayout layout) {
    vkDestroyDescriptorSetLayout(GlobalVulkanState.Device, layout, nullptr);
}


VkDescriptorPool vk::CreateDescriptorPool(VkDescriptorPoolSize *pool_sizes,
                                          u32 pool_size_count,
                                          u32 max_sets,
                                          VkDescriptorPoolCreateFlags flags) {
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = pool_size_count;
    poolInfo.pPoolSizes    = pool_sizes;
    poolInfo.maxSets       = max_sets;
    poolInfo.flags         = flags;
    
    VkDescriptorPool descriptor_pool;
    VK_CHECK_RESULT(vkCreateDescriptorPool(GlobalVulkanState.Device, &poolInfo, nullptr, &descriptor_pool),
                    "Failed to create descriptor pool!");
    
    return descriptor_pool;
}

void vk::DestroyDescriptorPool(VkDescriptorPool descriptor_pool) {
    vkDestroyDescriptorPool(GlobalVulkanState.Device, descriptor_pool, nullptr);
}

void vk::ResetDescriptorPool(VkDescriptorPool descriptor_pool)
{
    VkDescriptorPoolResetFlags flags = {};
    
    vkResetDescriptorPool(GlobalVulkanState.Device, descriptor_pool, flags);
}


VkResult vk::CreateDescriptorSets(VkDescriptorSet *descriptor_sets,
                                  VkDescriptorSetAllocateInfo allocInfo) {
    VkResult Result = vkAllocateDescriptorSets(GlobalVulkanState.Device,
                                               &allocInfo,
                                               descriptor_sets);
    return Result;
    
}

void vk::DestroyDescritporSets(VkDescriptorPool descriptor_pool,
                               VkDescriptorSet *descriptor_sets,
                               u32 descriptor_count) {
    vkFreeDescriptorSets(GlobalVulkanState.Device,
                         descriptor_pool,
                         descriptor_count,
                         descriptor_sets);
}

void vk::UpdateDescriptorSets(VkWriteDescriptorSet *descriptor_set_writes,
                              u32 write_count,
                              u32 copy_count,
                              VkCopyDescriptorSet* pstop) {
    vkUpdateDescriptorSets(GlobalVulkanState.Device,
                           write_count,
                           descriptor_set_writes,
                           copy_count,
                           pstop);
}

size_t vk::GetMinUniformMemoryOffsetAlignment() {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(GlobalVulkanState.PhysicalDevice, &properties);
    
    return properties.limits.minUniformBufferOffsetAlignment;
}


void vk::Draw(VkCommandBuffer command_buffer,
              u32             vertex_count,
              u32             instance_count,
              u32             first_vertex,
              u32             first_instance) {
    vkCmdDraw(command_buffer, vertex_count, instance_count, first_vertex, first_instance);
}

void vk::DrawIndexed(VkCommandBuffer command_buffer,
                     u32             index_count,
                     u32             instance_count,
                     u32             first_index,
                     u32             vertex_offset,
                     u32             first_instance) {
    vkCmdDrawIndexed(command_buffer,
                     index_count,
                     instance_count,
                     first_index,
                     vertex_offset,
                     first_instance);
}

void vk::BindVertexBuffers(VkCommandBuffer command_buffer,
                           u32             first_binding,
                           u32             binding_count,
                           VkBuffer        *buffers,
                           VkDeviceSize    *offsets) {
    vkCmdBindVertexBuffers(command_buffer, first_binding, binding_count, buffers, offsets);
}

void vk::BindIndexBuffer(VkCommandBuffer command_buffer,
                         VkBuffer        buffer,
                         VkDeviceSize    offset,
                         VkIndexType     index_type) {
    vkCmdBindIndexBuffer(command_buffer, buffer, offset, index_type);
}

void vk::BindDescriptorSets(VkCommandBuffer  command_buffer,
                            VkPipelineLayout layout,
                            u32              first_set,
                            u32              descriptor_set_count,
                            VkDescriptorSet  *descriptor_sets,
                            u32              dynamic_offset_count,
                            u32              *dynamic_offsets) {
    vkCmdBindDescriptorSets(command_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            layout,
                            first_set,
                            descriptor_set_count,
                            descriptor_sets,
                            dynamic_offset_count,
                            dynamic_offsets);
}

void vk::CopyBufferToImage(VkCommandPool command_pool,
                           VkBuffer buffer,
                           VkImage image,
                           u32 width, u32 height) {
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
    
    vkCmdCopyBufferToImage(command_buffer,
                           buffer,
                           image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &region);
    
    EndSingleTimeCommands(command_buffer, command_pool);
}

void vk::GenerateMipmaps(VkCommandPool command_pool,
                         VkImage image, VkFormat image_format,
                         i32 width, i32 height, u32 mip_levels) {
    
    // Determine if image format supports linear blitting
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(GlobalVulkanState.PhysicalDevice,
                                        image_format,
                                        &format_properties);
    if (!(format_properties.optimalTilingFeatures &
          VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        // NOTE(Dustin): Silent failure. Should probably have a resolution
        // strategy
        mprinte("Linear blitting not supported!\n");
    }
    
    VkCommandBuffer command_buffer = BeginSingleTimeCommands(command_pool);
    
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image                           = image;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.subresourceRange.levelCount     = 1;
    
    i32 mip_width = width;
    i32 mip_height = height;
    
    for (u32 i = 1; i < mip_levels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        
        vkCmdPipelineBarrier(command_buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
        
        VkImageBlit blit = {};
        blit.srcOffsets[0]                 = { 0, 0, 0 };
        blit.srcOffsets[1]                 = { mip_width, mip_height, 1 };
        blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel       = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount     = 1;
        blit.dstOffsets[0]                 = { 0, 0, 0 };
        blit.dstOffsets[1]                 = {
            mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1
        };
        blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel       = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount     = 1;
        
        vkCmdBlitImage(command_buffer,
                       image,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit,
                       VK_FILTER_LINEAR);
        
        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        vkCmdPipelineBarrier(command_buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
        
        if (mip_width > 1) mip_width /= 2;
        if (mip_height > 1) mip_height /= 2;
    }
    
    barrier.subresourceRange.baseMipLevel = mip_levels - 1;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
    
    EndSingleTimeCommands(command_buffer, command_pool);
}

VkSampleCountFlagBits vk::GetMaxMsaaSamples() {
    return GlobalVulkanState.MsaaSamples;
}