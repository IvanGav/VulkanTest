#pragma once

#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <chrono>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef uintptr_t usize;
typedef float f32;
typedef double f64;

namespace engine {

template <typename... Args>
void err(Args&&... strs) {
    std::cout << "ERROR: ";
    ((std::cout << std::forward<Args>(strs) << " "), ...);
    std::cout << std::endl;
    exit(1);
}

// Ensure that the result is `VK_SUCCESS`; raise an unrecoverable error when it's not
void success(VkResult result, std::string errorMessage = "No Message Provided") {
    if (result == VK_SUCCESS) return;
    err(errorMessage, string_VkResult(result));
}

struct BufferRef {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mappedData = nullptr;
    VkDeviceAddress deviceAddress = 0;
};

struct ImageRef {
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
};

struct ShaderUniformData {
    glm::mat4 proj;
    glm::mat4 view;
    glm::mat4 model;
};

std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}}
};

std::vector<u16> indices = {
    0, 1, 2, 2, 3, 0
};

/* global settings */
std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_UNIFIED_IMAGE_LAYOUTS_EXTENSION_NAME, VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME };
std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
//std::vector<const char*> validationLayers = {};
u32 framesInFlight = 2;

/* general */
u32 frameIndex = 0;

/* glfw */
GLFWwindow* window = nullptr;

/* vulkan */
VkInstance instance = VK_NULL_HANDLE;
VkSurfaceKHR surface = VK_NULL_HANDLE;
VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;
u32 queueFamily = 0;
VkQueue graphicsQueue = VK_NULL_HANDLE;
VkQueue presentQueue = VK_NULL_HANDLE;
VkSwapchainKHR swapchain = VK_NULL_HANDLE;
VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
VkExtent2D swapchainExtent = {};
ImageRef depthImage = {};
VkFormat depthImageFormat = VK_FORMAT_UNDEFINED;
BufferRef vertexBuffer = {};
BufferRef indexBuffer = {};
VkCommandPool commandPool = VK_NULL_HANDLE;
std::vector<ImageRef> textures = {};
std::vector<VkDescriptorImageInfo> textureDescriptors = {};
VkSampler sampler = VK_NULL_HANDLE;
VkDescriptorSetLayout texturesDescriptorSetLayout = VK_NULL_HANDLE;
VkDescriptorSet texturesDescriptorSet = VK_NULL_HANDLE;
VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
VkPipeline pipeline = VK_NULL_HANDLE;
VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
/* size = number of swapchain images */
std::vector<VkImage> swapchainImages = {};
std::vector<VkImageView> swapchainImageViews = {};
std::vector<VkSemaphore> renderCompleteSemaphores = {};
/* size = number of frames in flight */
std::vector<BufferRef> shaderBuffers = {};
std::vector<VkCommandBuffer> commandBuffers = {};
std::vector<VkFence> fences = {};
std::vector<VkSemaphore> imageAcquiredSemaphores = {};

/*
    Helper
*/

u32 findMemoryType(VkPhysicalDevice physicalDevice, u32 typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return UINT32_MAX;
}

void createBufferAndMemory(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memoryProperties,
    VkBuffer& outBuffer,
    VkDeviceMemory& outMemory,
    void** outMappedPointer
) {
    // 1. Create VkBuffer handle
    VkBufferCreateInfo bufferCI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    success(vkCreateBuffer(device, &bufferCI, nullptr, &outBuffer), "Could not create buffer");

    // 2. Query memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, outBuffer, &memRequirements);

    // 3. Find suitable memory type index
    u32 memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, memoryProperties);
    if (memoryTypeIndex == UINT32_MAX) {
        success(VK_ERROR_FEATURE_NOT_PRESENT, "Could not find suitable memory type for buffer");
    }

    // 4. Handle Buffer Device Address flag requirement if needed
    VkMemoryAllocateFlagsInfo allocFlagsInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO
    };
    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    }

    // 5. Allocate memory
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) ? &allocFlagsInfo : nullptr,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };
    success(vkAllocateMemory(device, &allocInfo, nullptr, &outMemory), "Could not allocate buffer memory");

    // 6. Bind buffer to allocated memory
    success(vkBindBufferMemory(device, outBuffer, outMemory, 0), "Could not bind buffer memory");

    // 7. Persistently map memory (matches VMA_ALLOCATION_CREATE_MAPPED_BIT)
    if (outMappedPointer != nullptr && (memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        success(vkMapMemory(device, outMemory, 0, size, 0, outMappedPointer), "Could not map buffer memory");
    }
}

ImageRef createImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImageAspectFlags aspectMask) {
    ImageRef image = {};
    u32 mipLevels = 1;
    VkImageCreateInfo imageCI = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {.width = width, .height = height, .depth = 1 },
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    success(vkCreateImage(device, &imageCI, nullptr, &image.image), "Could not create image");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image.image, &memRequirements);
    VkMemoryPropertyFlags memProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    memProperties |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // TODO check here

    u32 memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, memProperties);
    if (memoryTypeIndex == UINT32_MAX) {
        success(VK_ERROR_FEATURE_NOT_PRESENT, "Could not find suitable memory type for image");
    }

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = memoryTypeIndex,
    };
    success(vkAllocateMemory(device, &allocInfo, nullptr, &image.memory), "Could not allocate image memory");

    success(vkBindImageMemory(device, image.image, image.memory, 0), "Could not bind image memory");

    VkImageViewCreateInfo imageViewCI = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageCI.format,
        .subresourceRange = {.aspectMask = aspectMask, .levelCount = mipLevels, .layerCount = 1 }
    };
    success(vkCreateImageView(device, &imageViewCI, nullptr, &image.view), "Could not create image view");

    return image;
}

ImageRef loadImage(const char* path) {
    i32 texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (stbi_failure_reason()) {
        err("stbi", stbi_failure_reason());
    }

    VkDeviceSize imageSize = (u64)texWidth * (u64)texHeight * 4; // 4 bytes per pixel in VK_FORMAT_R8G8B8A8_SRGB

    ImageRef tex = createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_HOST_TRANSFER_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    VkMemoryToImageCopy memImgCopy{
        .sType = VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY,
        .pHostPointer = pixels,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageExtent = {
            .width = (u32)texWidth,
            .height = (u32)texHeight,
            .depth = 1,
        },
    };

    VkCopyMemoryToImageInfo memImgCopyInfo{
        .sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO,
        .dstImage = tex.image,
        .dstImageLayout = VK_IMAGE_LAYOUT_GENERAL,
        .regionCount = 1,
        .pRegions = &memImgCopy,
    };

    VkHostImageLayoutTransitionInfo imageLayoutTransitionInfo{
        .sType = VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO,
        .image = tex.image,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };
    vkTransitionImageLayout(device, 1, &imageLayoutTransitionInfo);
    std::cout << "about to crash?" << std::endl;
    vkCopyMemoryToImage(device, &memImgCopyInfo); // TODO renderdoc crashes here
    stbi_image_free(pixels);
    return tex;
}

// Minimal setup assumptions:
// - VkDevice created with VkPhysicalDeviceHostImageCopyFeatures::hostImageCopy = VK_TRUE
// - VkImage created with VK_IMAGE_USAGE_HOST_TRANSFER_BIT
// - Image memory allocated with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
void reproHostImageCopy(VkDevice device, VkImage image, void* pixelData, uint32_t width, uint32_t height) {
    VkMemoryToImageCopy region{
        .sType = VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY,
        .pHostPointer = pixelData,
        .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1 },
        .imageExtent = { .width = width, .height = height, .depth = 1 },
    };

    VkCopyMemoryToImageInfo info{
        .sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO,
        .dstImage = image,
        .dstImageLayout = VK_IMAGE_LAYOUT_GENERAL,
        .regionCount = 1,
        .pRegions = &region,
    };

    // When capturing with RenderDco, crashes inside nvoglv64.dll
    vkCopyMemoryToImage(device, &info);
}

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) { err("Could not open file", filename); }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

bool isWindowMinimized() {
    assert(window != nullptr);
    i32 width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    return width == 0 || height == 0;
}

VkPhysicalDevice selectPhysicalDevice(std::vector<VkPhysicalDevice>& devices) {
    for (u32 i = 0; i < devices.size(); i++) {
        VkPhysicalDeviceProperties2 p;
        vkGetPhysicalDeviceProperties2(devices[i], &p);
        std::cout << i << ": " << p.properties.deviceName << std::endl;
    }
    std::cout << "Which to use? ";
    u32 choice;
    std::cin >> choice;
    return devices[choice];
}

/*
    Init
*/

// create `window`
void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(500, 500, "Vulkan", nullptr, nullptr);
}

// create `instance`
void createInstance() {
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_4
    };

    u32 instanceExtensionsCount;
    const char** instanceExtensions = glfwGetRequiredInstanceExtensions(&instanceExtensionsCount);

    VkInstanceCreateInfo instanceCI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = (u32)validationLayers.size(),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = instanceExtensionsCount,
        .ppEnabledExtensionNames = instanceExtensions,
    };

    VkResult result = vkCreateInstance(&instanceCI, nullptr, &instance);
    if (result != VK_SUCCESS) { err("Could not create instance", string_VkResult(result)); }
}

// create `physicalDevice`
void createPhysicalDevice() {
    u32 deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices; devices.resize(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    if (devices.size() == 1) { physicalDevice = devices[0]; }
    else { physicalDevice = selectPhysicalDevice(devices); }
}

// create `surface` and `surfaceCapabilities`
void createSurface() {
    success(glfwCreateWindowSurface(instance, window, nullptr, &surface), "Could not create window surface");
    success(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities), "Could not get surface capabilities");
}

// create `device`
void createDevice(VkPhysicalDevice physicalDevice) {
    assert(physicalDevice != VK_NULL_HANDLE);
    u32 queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies; queueFamilies.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    queueFamily = 0xFFFFFFFF;
    for (u32 i = 0; i < queueFamilies.size(); i++) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        // don't handle if they're different queues
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport) {
            queueFamily = i;
            break;
        }
    }
    if (queueFamily == 0xFFFFFFFF) { err("Could not find acceptable queue family"); }

    const f32 queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    VkPhysicalDeviceVulkan12Features enabledVk12Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = VK_TRUE,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE, // related to descriptor indexing
        .descriptorBindingVariableDescriptorCount = VK_TRUE, // related to descriptor indexing
        .runtimeDescriptorArray = VK_TRUE, // related to descriptor indexing
        .bufferDeviceAddress = VK_TRUE,
    };
    VkPhysicalDeviceVulkan13Features enabledVk13Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &enabledVk12Features,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };
    VkPhysicalDeviceVulkan14Features enabledVk14Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pNext = &enabledVk13Features,
        .maintenance5 = VK_TRUE,
        .hostImageCopy = VK_TRUE,
    };
    VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR unifiedImageLayousFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFIED_IMAGE_LAYOUTS_FEATURES_KHR,
        .pNext = &enabledVk14Features,
        .unifiedImageLayouts = VK_TRUE,
    };
    VkPhysicalDeviceFeatures enabledVk10Features{
        .samplerAnisotropy = VK_TRUE,
    };

    VkDeviceCreateInfo deviceCI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &unifiedImageLayousFeatures,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCI,
        .enabledExtensionCount = (u32)deviceExtensions.size(),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &enabledVk10Features
    };
    VkResult result = vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device);
    if (result != VK_SUCCESS) { err("Could not create logical device", result); }
    vkGetDeviceQueue(device, queueFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueFamily, 0, &presentQueue);
}

// create `swapchain`, `swapchainImageFormat`, `swapchainExtent`, `swapchainImages`, `swapchainImageViews`
void createSwapchain() {
    swapchainExtent = surfaceCapabilities.currentExtent;
    if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) { err("no"); }
    swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    VkSwapchainCreateInfoKHR swapchainCI{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = surfaceCapabilities.minImageCount,
        .imageFormat = swapchainImageFormat,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = swapchainExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .oldSwapchain = swapchain
    };
    success(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain), "Could not create swapchain");

    u32 imageCount;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
    swapchainImageViews.resize(imageCount);

    for (u32 i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo viewCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchainImageFormat,
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
        };
        success(vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]), "Could not create image view");
    }
}

// create `depthImage`, `depthImageAllocation`, `depthImageView`
void createDepthImage() { // TODO AI warning
    std::vector<VkFormat> depthFormatList = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    for (VkFormat& format : depthFormatList) {
        VkFormatProperties2 formatProperties{ .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
        vkGetPhysicalDeviceFormatProperties2(physicalDevice, format, &formatProperties);
        if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            depthFormat = format;
            break;
        }
    }
    depthImageFormat = depthFormat;

    VkImageCreateInfo depthImageCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depthFormat,
        .extent = {.width = swapchainExtent.width, .height = swapchainExtent.height, .depth = 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    success(vkCreateImage(device, &depthImageCI, nullptr, &depthImage.image), "Could not create image (depth buffer)");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, depthImage.image, &memRequirements);

    u32 memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memoryTypeIndex == UINT32_MAX) {
        success(VK_ERROR_FEATURE_NOT_PRESENT, "Could not find suitable memory type for depth image");
    }

    VkMemoryDedicatedAllocateInfo dedicatedAllocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
        .image = depthImage.image,
        .buffer = VK_NULL_HANDLE
    };

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &dedicatedAllocInfo, // TODO check
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = memoryTypeIndex,
    };
    success(vkAllocateMemory(device, &allocInfo, nullptr, &depthImage.memory), "Could not allocate depth image memory");

    success(vkBindImageMemory(device, depthImage.image, depthImage.memory, 0), "Could not bind depth image memory");

    VkImageViewCreateInfo depthViewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = depthImage.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depthFormat,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 }
    };
    success(vkCreateImageView(device, &depthViewCI, nullptr, &depthImage.view), "Could not create image view (depth buffer)");
}

// create `vertexBuffer`, `indexBuffer`, `shaderBuffers`
void createBuffers() {
    // Memory flags equivalent to VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT:
    // HOST_VISIBLE allows host mapping, HOST_COHERENT avoids manual flush calls (vkFlushMappedMemoryRanges).
    VkMemoryPropertyFlags hostMemFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    // 1. Vertex Buffer
    createBufferAndMemory(
        vertices.size() * sizeof(Vertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        hostMemFlags,
        vertexBuffer.buffer,
        vertexBuffer.memory,
        &vertexBuffer.mappedData
    );

    // 2. Index Buffer
    createBufferAndMemory(
        indices.size() * sizeof(u16),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        hostMemFlags,
        indexBuffer.buffer,
        indexBuffer.memory,
        &indexBuffer.mappedData
    );

    // 3. Shader Uniform Buffers (with Buffer Device Address)
    shaderBuffers.resize(framesInFlight);
    for (u32 i = 0; i < framesInFlight; i++) {
        createBufferAndMemory(
            sizeof(ShaderUniformData),
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            hostMemFlags,
            shaderBuffers[i].buffer,
            shaderBuffers[i].memory,
            &shaderBuffers[i].mappedData
        );

        // Fetch Buffer Device Address
        VkBufferDeviceAddressInfo uBufferBdaInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = shaderBuffers[i].buffer
        };
        shaderBuffers[i].deviceAddress = vkGetBufferDeviceAddress(device, &uBufferBdaInfo);
    }
}

// create `fences`, imageAcquiredSemaphores`, `renderCompleteSemaphores`
void createSyncStructures() {
    VkSemaphoreCreateInfo semaphoreCI{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceCI{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    fences.resize(framesInFlight);
    imageAcquiredSemaphores.resize(framesInFlight);
    for (u32 i = 0; i < framesInFlight; i++) {
        success(vkCreateFence(device, &fenceCI, nullptr, &fences[i]), "Could not create fence");
        success(vkCreateSemaphore(device, &semaphoreCI, nullptr, &imageAcquiredSemaphores[i]), "Could not create semaphore");
    }
    renderCompleteSemaphores.resize(swapchainImages.size());
    for (VkSemaphore& semaphore : renderCompleteSemaphores) {
        success(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore), "Could not create semaphore");
    }
}

// create `commandPool`, `commandBuffers`
void createCommandBuffers() {
    VkCommandPoolCreateInfo commandPoolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamily
    };
    success(vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool), "Could not create command pool");

    VkCommandBufferAllocateInfo cbAllocCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .commandBufferCount = framesInFlight
    };
    commandBuffers.resize(framesInFlight);
    success(vkAllocateCommandBuffers(device, &cbAllocCI, commandBuffers.data()), "Could not create command buffers");
}

// create `sampler`
void createSamplers() {
    VkSamplerCreateInfo samplerCI{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 8.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
    };
    success(vkCreateSampler(device, &samplerCI, nullptr, &sampler), "Could not create sampler");
}

// create `texturesDescriptorSetLayout`, `descriptorPool`, `texturesDescriptorSet`
void createDescriptorSetsForTextures() {
    VkDescriptorBindingFlags descVariableFlag{ VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT };
    VkDescriptorSetLayoutBindingFlagsCreateInfo descBindingFlags{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 1,
        .pBindingFlags = &descVariableFlag,
    };
    VkDescriptorSetLayoutBinding descLayoutBindingTex{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = (u32)textures.size(),
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    VkDescriptorSetLayoutCreateInfo descLayoutTexCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &descBindingFlags,
        .bindingCount = 1,
        .pBindings = &descLayoutBindingTex,
    };
    {
        VkResult result = vkCreateDescriptorSetLayout(device, &descLayoutTexCI, nullptr, &texturesDescriptorSetLayout);
        if (result != VK_SUCCESS) { err("Cannot create descriptor set layout for textures", string_VkResult(result)); }
    }

    VkDescriptorPoolSize poolSize{
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = (u32)textures.size(),
    };
    VkDescriptorPoolCreateInfo descPoolCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };
    {
        VkResult result = vkCreateDescriptorPool(device, &descPoolCI, nullptr, &descriptorPool);
        if (result != VK_SUCCESS) { err("Cannot create descriptor pool for textures", string_VkResult(result)); }
    }

    u32 variableDescCount{ (u32)textures.size() };
    VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescCountAI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
        .descriptorSetCount = 1,
        .pDescriptorCounts = &variableDescCount
    };
    VkDescriptorSetAllocateInfo texDescSetAlloc{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &variableDescCountAI,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &texturesDescriptorSetLayout
    };
    success(vkAllocateDescriptorSets(device, &texDescSetAlloc, &texturesDescriptorSet), "Cannot create descriptor set for textures");

    VkWriteDescriptorSet writeDescSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = texturesDescriptorSet,
        .dstBinding = 0,
        .descriptorCount = (u32)textureDescriptors.size(),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = textureDescriptors.data()
    };
    vkUpdateDescriptorSets(device, 1, &writeDescSet, 0, nullptr);
}

// create `pipeline`, `pipelineLayout`
void createGraphicsPipeline() {
    std::vector<char> vertShaderCode = readFile("src/shader/vert.spv");
    std::vector<char> fragShaderCode = readFile("src/shader/frag.spv");
    VkShaderModuleCreateInfo vertShaderCI{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = (u32)vertShaderCode.size(),
        .pCode = (u32*)vertShaderCode.data(),
    };
    VkShaderModuleCreateInfo fragShaderCI{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = (u32)fragShaderCode.size(),
        .pCode = (u32*)fragShaderCode.data(),
    };
    VkPipelineShaderStageCreateInfo vertShaderStageCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = &vertShaderCI,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .pName = "main",
    };
    VkPipelineShaderStageCreateInfo fragShaderStageCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = &fragShaderCI,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pName = "main",
    };

    VkPushConstantRange pushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .size = sizeof(VkDeviceAddress)
    };
    VkPipelineLayoutCreateInfo pipelineLayoutCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &texturesDescriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };
    success(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout), "Could not create pipeline layout");
    // Describe the `struct Vertex`
    VkVertexInputBindingDescription vertexBinding{
         .binding = 0,
         .stride = sizeof(Vertex),
         .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    std::vector<VkVertexInputAttributeDescription> vertexAttributes{
        {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos) },
        {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv) }
    };
    // All the structures required to create the pipeline
    VkPipelineVertexInputStateCreateInfo vertexInputStateCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexBinding,
        .vertexAttributeDescriptionCount = (u32)vertexAttributes.size(),
        .pVertexAttributeDescriptions = vertexAttributes.data(),
    };
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertShaderStageCI, fragShaderStageCI };
    VkPipelineViewportStateCreateInfo viewportStateCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };
    std::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicStateCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = (u32)dynamicStates.size(),
        .pDynamicStates = dynamicStates.data(),
    };
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
    };
    VkPipelineRenderingCreateInfo renderingCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapchainImageFormat,
        .depthAttachmentFormat = depthImageFormat
    };
    VkPipelineColorBlendAttachmentState blendAttachment{
        .colorWriteMask = 0xF
    };
    VkPipelineColorBlendStateCreateInfo colorBlendStateCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blendAttachment
    };
    VkPipelineRasterizationStateCreateInfo rasterizationStateCI{
         .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
         .lineWidth = 1.0f
    };
    VkPipelineMultisampleStateCreateInfo multisampleStateCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };
    VkGraphicsPipelineCreateInfo pipelineCI{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingCI,
        .stageCount = (u32)shaderStages.size(),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputStateCI,
        .pInputAssemblyState = &inputAssemblyStateCI,
        .pViewportState = &viewportStateCI,
        .pRasterizationState = &rasterizationStateCI,
        .pMultisampleState = &multisampleStateCI,
        .pDepthStencilState = &depthStencilStateCI,
        .pColorBlendState = &colorBlendStateCI,
        .pDynamicState = &dynamicStateCI,
        .layout = pipelineLayout
    };
    success(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline), "Could not create graphics pipeline");
}

void recreateSwapchain() {
    while (isWindowMinimized()) { glfwWaitEvents(); }
    success(vkDeviceWaitIdle(device));
    success(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities), "Could not get surface capabilities");
    for (u32 i = 0; i < (u32)swapchainImageViews.size(); i++) {
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    }
    vkDestroyImageView(device, depthImage.view, nullptr);
    vkDestroyImage(device, depthImage.image, nullptr);
    vkFreeMemory(device, depthImage.memory, nullptr);
    VkSwapchainKHR oldSwapchain = swapchain;
    createSwapchain();
    createDepthImage();
    vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
    for (VkSemaphore& semaphore : renderCompleteSemaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    renderCompleteSemaphores.resize(swapchainImages.size());
    VkSemaphoreCreateInfo semaphoreCI{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    for (VkSemaphore& semaphore : renderCompleteSemaphores) {
        success(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
    }
}

/*
    Public
*/

void loadTexture(const char* path) {
    assert(sampler != VK_NULL_HANDLE);
    textures.push_back(loadImage(path));
    textureDescriptors.push_back(VkDescriptorImageInfo{
        .sampler = sampler,
        .imageView = textures.back().view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    });
}

void init() {
    initWindow();
    createInstance();
    createPhysicalDevice();
    createSurface();
    createDevice(physicalDevice);
    createSwapchain();
    createDepthImage();
    createBuffers();
    createSyncStructures();
    createCommandBuffers();
    createSamplers();
    loadTexture("asset/vulkan.png");
    createDescriptorSetsForTextures();
    createGraphicsPipeline();
    memcpy(vertexBuffer.mappedData, vertices.data(), vertices.size() * sizeof(Vertex));
    memcpy(indexBuffer.mappedData, indices.data(), indices.size() * sizeof(u16));
}

void cleanup() {
    vkDeviceWaitIdle(device);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, texturesDescriptorSetLayout, nullptr);
    for (ImageRef tex : textures) {
        vkDestroyImageView(device, tex.view, nullptr);
        vkDestroyImage(device, tex.image, nullptr);
        vkFreeMemory(device, tex.memory, nullptr);
    }
    vkDestroySampler(device, sampler, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    for (VkFence fence : fences) {
        vkDestroyFence(device, fence, nullptr);
    }
    for (VkSemaphore sem : imageAcquiredSemaphores) {
        vkDestroySemaphore(device, sem, nullptr);
    }
    for (VkSemaphore sem : renderCompleteSemaphores) {
        vkDestroySemaphore(device, sem, nullptr);
    }
    for (BufferRef buffer : shaderBuffers) {
        vkUnmapMemory(device, buffer.memory);
        vkDestroyBuffer(device, buffer.buffer, nullptr);
    }
    vkUnmapMemory(device, indexBuffer.memory);
    vkDestroyBuffer(device, indexBuffer.buffer, nullptr);
    vkUnmapMemory(device, vertexBuffer.memory);
    vkDestroyBuffer(device, vertexBuffer.buffer, nullptr);
    vkDestroyImageView(device, depthImage.view, nullptr);
    vkDestroyImage(device, depthImage.image, nullptr);
    vkFreeMemory(device, depthImage.memory, nullptr);
    for (VkImageView view : swapchainImageViews) {
        vkDestroyImageView(device, view, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void drawFrame() {
    success(vkWaitForFences(device, 1, &fences[frameIndex], VK_TRUE, UINT64_MAX));
    u32 imageIndex;
    {
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAcquiredSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) { recreateSwapchain(); }
        else { success(result, "Could not acquire swapchain image"); }
    }
    success(vkResetFences(device, 1, &fences[frameIndex]));

    // Put in uniform data
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        ShaderUniformData uniformData = {
            .proj = glm::perspective(glm::radians(45.0f), engine::swapchainExtent.width / (f32)engine::swapchainExtent.height, 0.1f, 10.0f),
            .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::normalize(glm::vec3(0.0f, 0.5f, 1.0f))),
        };
        uniformData.proj[1][1] *= -1;

        memcpy(shaderBuffers[frameIndex].mappedData, &uniformData, sizeof(ShaderUniformData));
    }

    // Render
    VkCommandBuffer cb = commandBuffers[frameIndex];
    success(vkResetCommandBuffer(cb, 0));
    VkCommandBufferBeginInfo cbBI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    success(vkBeginCommandBuffer(cb, &cbBI));
    std::vector<VkImageMemoryBarrier2> outputBarriers{
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .image = swapchainImages[imageIndex],
            .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
        },
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .image = depthImage.image,
            .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .levelCount = 1, .layerCount = 1 }
        }
    };
    VkDependencyInfo barrierDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = (u32)outputBarriers.size(),
        .pImageMemoryBarriers = outputBarriers.data()
    };
    vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);

    VkRenderingAttachmentInfo colorAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchainImageViews[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = { .color = { 0.0f, 0.0f, 0.2f, 1.0f } }
    };
    VkRenderingAttachmentInfo depthAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = depthImage.view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue = { .depthStencil = {1.0f,  0} }
    };
    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = { .extent = swapchainExtent },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo
    };
    vkCmdBeginRendering(cb, &renderingInfo);

    VkViewport vp{ .width = (f32)swapchainExtent.width, .height = (f32)swapchainExtent.height, .minDepth = 0.0f, .maxDepth = 1.0f };
    vkCmdSetViewport(cb, 0, 1, &vp);
    VkRect2D scissor{ .extent = swapchainExtent };
    vkCmdSetScissor(cb, 0, 1, &scissor);
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &texturesDescriptorSet, 0, nullptr);
    VkDeviceSize zeroOffset = 0;
    vkCmdBindVertexBuffers(cb, 0, 1, &vertexBuffer.buffer, &zeroOffset);
    vkCmdBindIndexBuffer(cb, indexBuffer.buffer, zeroOffset, VK_INDEX_TYPE_UINT16);
    vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &shaderBuffers[frameIndex].deviceAddress);
    vkCmdDrawIndexed(cb, indices.size(), 1, 0, 0, 0);
    vkCmdEndRendering(cb);

    VkImageMemoryBarrier2 barrierPresent{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = swapchainImages[imageIndex],
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
    };
    VkDependencyInfo barrierPresentDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrierPresent
    };
    vkCmdPipelineBarrier2(cb, &barrierPresentDependencyInfo);
    vkEndCommandBuffer(cb);

    VkSemaphoreSubmitInfo waitSemaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = imageAcquiredSemaphores[frameIndex],
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    VkCommandBufferSubmitInfo commandBufferSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cb
    };
    VkSemaphoreSubmitInfo signalSemaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = renderCompleteSemaphores[imageIndex],
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    VkSubmitInfo2 submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &waitSemaphoreInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &commandBufferSubmitInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signalSemaphoreInfo,
    };
    success(vkQueueSubmit2(graphicsQueue, 1, &submitInfo, fences[frameIndex]), "Could not submit queue");

    frameIndex = (frameIndex + 1) % framesInFlight;

    // Present
    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderCompleteSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex
    };
    {
        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) { recreateSwapchain(); }
        else { success(result, "Could not present swap chain image"); }
    }
}
}