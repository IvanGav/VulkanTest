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

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#define TINYOBJLOADER_DISABLE_FAST_FLOAT
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "core/prelude.h"
#include "core/slice.h"
#include "core/vec.h"
#include "core/str.h"

namespace data {
    void framebufferResizedCallback();
}

namespace engine {

// TODO
#define MAGIC_NUMBER 1024

// Ensure that the result is `VK_SUCCESS`; raise an unrecoverable error when it's not
void success(VkResult result, std::string errorMessage = "No Message Provided") {
    if (result == VK_SUCCESS) return;
    err(errorMessage, string_VkResult(result));
}

struct BufferRef {
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;
    VkBuffer buffer;
    VkDeviceAddress deviceAddress;
};

struct ImageRef {
    VmaAllocation allocation;
    VkImage image;
    VkImageView view;
};

typedef u32 TextureRef;

struct MeshRef {
    u32 ioff;
    u32 size;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 normal;
};

struct alignas(16) ShaderUniformData {
    glm::mat4 proj;
    glm::mat4 view;
    glm::vec3 camPosition;
    u32 pad1[1];
};

struct alignas(16) ShaderInstanceData {
    glm::mat4 model;
    TextureRef texture;
    u32 pad1[3];
};

/* data */
std::vector<Vertex> vertices = {};
std::vector<u16> indices = {};
std::vector<const char*> texturePaths = {};

/* global settings - set up before calling `init` */
std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_UNIFIED_IMAGE_LAYOUTS_EXTENSION_NAME };
std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
u32 framesInFlight = 3;

/* general */
u32 frameIndex = 0;
u32 imageIndex = 0;
u32 instanceCount = 0;
VkCommandBuffer activeDrawCommandBuffer = VK_NULL_HANDLE;
bool initialized = false;

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
VmaAllocator allocator = VK_NULL_HANDLE;
VkSwapchainKHR swapchain = VK_NULL_HANDLE;
VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
VkExtent2D swapchainExtent = {};
ImageRef depthImage = {};
VkFormat depthImageFormat = VK_FORMAT_UNDEFINED;
BufferRef vertexBuffer = {};
BufferRef indexBuffer = {};
VkCommandPool commandPool = VK_NULL_HANDLE; // TODO Create one per thread
std::vector<ImageRef> textures = {};
std::vector<VkDescriptorImageInfo> textureDescriptors = {}; // TODO probably just create in the descriptor sets function
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

ImageRef createImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImageAspectFlags aspectMask) {
    ImageRef image = {};
    u32 mipLevels = 1;
    VkImageCreateInfo imageCI = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = { .width = width, .height = height, .depth = 1 },
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo imageAllocCI = { .usage = VMA_MEMORY_USAGE_AUTO };
    success(vmaCreateImage(allocator, &imageCI, &imageAllocCI, &image.image, &image.allocation, nullptr), "Could not create image");

    VkImageViewCreateInfo imageViewCI = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageCI.format,
        .subresourceRange = { .aspectMask = aspectMask, .levelCount = mipLevels, .layerCount = 1 }
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
    vkCopyMemoryToImage(device, &memImgCopyInfo);
    stbi_image_free(pixels);
    return tex;
}

std::vector<u8> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) { err("Could not open file", filename); }

    size_t fileSize = (size_t)file.tellg();
    std::vector<u8> buffer(fileSize);

    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);

    file.close();

    return buffer;
}

bool isWindowMinimized() {
    assert(window != nullptr);
    i32 width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    return width == 0 || height == 0;
}

VkPhysicalDevice selectPhysicalDevice(Slice<VkPhysicalDevice> devices) {
    for (u32 i = 0; i < devices.size; i++) {
        VkPhysicalDeviceProperties2 p = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        vkGetPhysicalDeviceProperties2(devices[i], &p);
        std::cout << i << ": " << p.properties.deviceName << std::endl;
    }
    std::cout << "Which to use? ";
    u32 choice;
    std::cin >> choice;
    return devices[choice];
}

void createTexture(const char* path) {
    assert(sampler != VK_NULL_HANDLE);
    textures.push_back(loadImage(path));
    textureDescriptors.push_back(VkDescriptorImageInfo{
        .sampler = sampler,
        .imageView = textures.back().view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    });
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
    // TODO this can help with visuals *during* resizing the window //glfwSetWindowUserPointer(window, this); //glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

// create `instance`
void createInstance() {
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "How to Vulkan",
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

    if (devices.size() == 1) {
        physicalDevice = devices[0];
    }
    else {
        physicalDevice = selectPhysicalDevice(Slice<VkPhysicalDevice>::from(devices));
    }
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

    queueFamily = U32_MAX;
    for (u32 i = 0; i < queueFamilies.size(); i++) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        // don't want to deal with them if they're different queues; all normal gpus should have them as one queue family; fix later if needed
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport) {
            queueFamily = i;
            break;
        }
    }
    if (queueFamily == U32_MAX) { err("Could not find acceptable queue family"); }

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

// create `allocator`
void initVma() {
    VmaVulkanFunctions vkFunctions{
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        .vkCreateImage = vkCreateImage
    };
    VmaAllocatorCreateInfo allocatorCI{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = physicalDevice,
        .device = device,
        .pVulkanFunctions = &vkFunctions,
        .instance = instance
    };
    VkResult result = vmaCreateAllocator(&allocatorCI, &allocator);
    if (result != VK_SUCCESS) { err("Could not create VMA allocator", string_VkResult(result)); }
}

// create `swapchain`, `swapchainImageFormat`, `swapchainExtent`, `swapchainImages`, `swapchainImageViews`
void createSwapchain() {
    swapchainExtent = surfaceCapabilities.currentExtent;
    if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) { err("Wayland moment"); }

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
        .presentMode = VK_PRESENT_MODE_FIFO_KHR, // aka v-sync
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
void createDepthImage() {
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
    VmaAllocationCreateInfo allocCI{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    success(vmaCreateImage(allocator, &depthImageCI, &allocCI, &depthImage.image, &depthImage.allocation, nullptr), "Could not create image (depth buffer)");

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
    VmaAllocationCreateInfo bufferAllocCI{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    VkBufferCreateInfo vBufferCI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = vertices.size() * sizeof(Vertex),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    };
    VkBufferCreateInfo iBufferCI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = indices.size() * sizeof(u16),
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    };
    success(vmaCreateBuffer(allocator, &vBufferCI, &bufferAllocCI, &vertexBuffer.buffer, &vertexBuffer.allocation, &vertexBuffer.allocationInfo), "Could not allocate vertex buffer");
    success(vmaCreateBuffer(allocator, &iBufferCI, &bufferAllocCI, &indexBuffer.buffer, &indexBuffer.allocation, &indexBuffer.allocationInfo), "Could not allocate index buffer");

    shaderBuffers.resize(framesInFlight);
    for (u32 i = 0; i < framesInFlight; i++) {
        VkBufferCreateInfo uBufferCI{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(ShaderUniformData) + MAGIC_NUMBER,
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        };
        success(vmaCreateBuffer(allocator, &uBufferCI, &bufferAllocCI, &shaderBuffers[i].buffer, &shaderBuffers[i].allocation, &shaderBuffers[i].allocationInfo), "Could not allocate buffer");
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
        {
            VkResult result = vkCreateFence(device, &fenceCI, nullptr, &fences[i]);
            if (result != VK_SUCCESS) { err("Could not create fence", string_VkResult(result)); }
        }
        {
            VkResult result = vkCreateSemaphore(device, &semaphoreCI, nullptr, &imageAcquiredSemaphores[i]);
            if (result != VK_SUCCESS) { err("Could not create semaphore", string_VkResult(result)); }
        }
    }
    renderCompleteSemaphores.resize(swapchainImages.size());
    for (VkSemaphore& semaphore : renderCompleteSemaphores) {
        VkResult result = vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore);
        if (result != VK_SUCCESS) { err("Could not create semaphore", string_VkResult(result)); }
    }
}

// create `commandPool`, `commandBuffers`
void createCommandBuffers() {
    VkCommandPoolCreateInfo commandPoolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamily
    };
    {
        VkResult result = vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool);
        if (result != VK_SUCCESS) { err("Could not create command pool"); }
    }

    VkCommandBufferAllocateInfo cbAllocCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .commandBufferCount = framesInFlight
    };
    commandBuffers.resize(framesInFlight);
    {
        VkResult result = vkAllocateCommandBuffers(device, &cbAllocCI, commandBuffers.data());
        if (result != VK_SUCCESS) { err("Could not create command buffers"); }
    }
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
    VkResult result = vkCreateSampler(device, &samplerCI, nullptr, &sampler);
    if (result != VK_SUCCESS) { err("Could not create sampler", string_VkResult(result)); }
}

// TODO [VK_EXT_descriptor_heap](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_descriptor_heap.html)
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
        .descriptorCount = (u32) textures.size(),
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
        .descriptorCount = (u32) textures.size(),
    };
    VkDescriptorPoolCreateInfo descPoolCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1, // how many descriptor sets
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };
    {
        VkResult result = vkCreateDescriptorPool(device, &descPoolCI, nullptr, &descriptorPool);
        if (result != VK_SUCCESS) { err("Cannot create descriptor pool for textures", string_VkResult(result)); }
    }

    u32 variableDescCount = textures.size();
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
        .descriptorCount = (u32) textureDescriptors.size(),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = textureDescriptors.data()
    };
    vkUpdateDescriptorSets(device, 1, &writeDescSet, 0, nullptr);
}

// create `pipeline`, `pipelineLayout`
void createGraphicsPipeline() {
    std::vector<u8> vertShaderCode = readFile("src/shader/vert.spv");
    std::vector<u8> fragShaderCode = readFile("src/shader/frag.spv");
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
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
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
        {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv) },
        {.location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal) }
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
    // `swapchainImages` are owned by `swapchain`, not `Engine`, so don't destroy them here
    for (u32 i = 0; i < (u32) swapchainImageViews.size(); i++) {
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    }
    vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
    vkDestroyImageView(device, depthImage.view, nullptr);
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
    data::framebufferResizedCallback();
}

/*
    Public
*/

TextureRef loadTexture(const char* path) {
    assert(!initialized);
    texturePaths.push_back(path);
    return texturePaths.size() - 1;
}

MeshRef loadMeshObj(const char* path) {
    assert(!initialized);
    MeshRef mesh = { .ioff = (u32) indices.size() };
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    {
        bool result = tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, path);
        if (!result) { err("Could not load obj"); }
    }

    for (auto& index : shapes[0].mesh.indices) {
        Vertex v = {
            .pos = { attrib.vertices[index.vertex_index * 3], -attrib.vertices[index.vertex_index * 3 + 2], attrib.vertices[index.vertex_index * 3 + 1] },
            .uv = { attrib.texcoords[index.texcoord_index * 2], 1.0 - attrib.texcoords[index.texcoord_index * 2 + 1] },
            .normal = { attrib.normals[index.normal_index * 3], -attrib.normals[index.normal_index * 3 + 2], attrib.normals[index.normal_index * 3 + 1] }
        };
        vertices.push_back(v);
        indices.push_back((u16)vertices.size() - 1);
    }
    mesh.size = u32(indices.size()) - mesh.ioff;
    return mesh;
}

MeshRef loadTestMesh() {
    assert(!initialized);
    MeshRef mesh = { .ioff = (u32)indices.size(), .size = 6 };
    vertices.push_back({ .pos = {0.0f,  0.5f,  0.5f}, .uv = {0.0f, 0.0f}, .normal = {1.0f, 0.0f, 0.0f} });
    vertices.push_back({ .pos = {0.0f, -0.5f,  0.5f}, .uv = {1.0f, 0.0f}, .normal = {1.0f, 0.0f, 0.0f} });
    vertices.push_back({ .pos = {0.0f, -0.5f, -0.5f}, .uv = {1.0f, 1.0f}, .normal = {1.0f, 0.0f, 0.0f} });
    vertices.push_back({ .pos = {0.0f,  0.5f, -0.5f}, .uv = {0.0f, 1.0f}, .normal = {1.0f, 0.0f, 0.0f} });
    indices.push_back(mesh.ioff + 0);
    indices.push_back(mesh.ioff + 1);
    indices.push_back(mesh.ioff + 2);
    indices.push_back(mesh.ioff + 2);
    indices.push_back(mesh.ioff + 3);
    indices.push_back(mesh.ioff + 0);
    return mesh;
}

void init() {
    initWindow();
    createInstance();
    createPhysicalDevice();
    createSurface();
    createDevice(physicalDevice);
    initVma();
    createSwapchain();
    createDepthImage();
    createBuffers();
    createSyncStructures();
    createCommandBuffers();
    createSamplers();
    for (const char* path : texturePaths) {
        createTexture(path);
    }
    createDescriptorSetsForTextures();
    createGraphicsPipeline();
    memcpy(vertexBuffer.allocationInfo.pMappedData, vertices.data(), vertices.size() * sizeof(Vertex));
    memcpy(indexBuffer.allocationInfo.pMappedData, indices.data(), indices.size() * sizeof(u16));
    initialized = true;
}

void cleanup() {
    vkDeviceWaitIdle(device);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, texturesDescriptorSetLayout, nullptr);
    for (ImageRef tex : textures) {
        vkDestroyImageView(device, tex.view, nullptr);
        vmaDestroyImage(allocator, tex.image, tex.allocation);
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
        vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
    }
    vmaDestroyBuffer(allocator, indexBuffer.buffer, indexBuffer.allocation);
    vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.allocation);
    vkDestroyImageView(device, depthImage.view, nullptr);
    vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
    for (VkImageView view : swapchainImageViews) {
        vkDestroyImageView(device, view, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void drawFrame(MeshRef singleMesh, ShaderUniformData& uniformData, ShaderInstanceData& instanceData) {
    success(vkWaitForFences(device, 1, &fences[frameIndex], VK_TRUE, UINT64_MAX));
    u32 imageIndex = U32_MAX;
    {
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAcquiredSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) { recreateSwapchain(); }
        else { success(result, "Could not acquire swapchain image"); }
    }
    success(vkResetFences(device, 1, &fences[frameIndex]));

    // Put in uniform data
    char* buf = (char*) (shaderBuffers[frameIndex].allocationInfo.pMappedData);
    memcpy(buf, &uniformData, sizeof(ShaderUniformData));
    // Put in the per-instance data (exactly 1 here)
    memcpy(buf + sizeof(ShaderUniformData), &instanceData, sizeof(ShaderInstanceData));

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
    vkCmdDrawIndexed(cb, singleMesh.size, 1, singleMesh.ioff, 0, 0);
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

void startDraw(ShaderUniformData& uniformData, Slice<ShaderInstanceData> instanceData) {
    assert(activeDrawCommandBuffer == VK_NULL_HANDLE);
    success(vkWaitForFences(device, 1, &fences[frameIndex], VK_TRUE, UINT64_MAX));
    imageIndex = U32_MAX;
    {
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAcquiredSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) { recreateSwapchain(); }
        else { success(result, "Could not acquire swapchain image"); }
    }
    success(vkResetFences(device, 1, &fences[frameIndex]));

    u8* buf = (u8*)(shaderBuffers[frameIndex].allocationInfo.pMappedData);
    memcpy(buf, &uniformData, sizeof(ShaderUniformData));
    memcpy(buf + sizeof(ShaderUniformData), instanceData.data, instanceData.size * sizeof(ShaderInstanceData));

    // Render
    VkCommandBuffer cb = commandBuffers[frameIndex];
    success(vkResetCommandBuffer(cb, 0));
    VkCommandBufferBeginInfo cbBI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    success(vkBeginCommandBuffer(cb, &cbBI));
    std::vector<VkImageMemoryBarrier2> outputBarriers = {
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .image = swapchainImages[imageIndex],
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
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
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .levelCount = 1, .layerCount = 1 }
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
        .clearValue = { .depthStencil = { 1.0f, 0 } }
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

    VkViewport vp = { .width = (f32)swapchainExtent.width, .height = (f32)swapchainExtent.height, .minDepth = 0.0f, .maxDepth = 1.0f };
    vkCmdSetViewport(cb, 0, 1, &vp);
    VkRect2D scissor{ .extent = swapchainExtent };
    vkCmdSetScissor(cb, 0, 1, &scissor);
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &texturesDescriptorSet, 0, nullptr);
    VkDeviceSize zeroOffset = 0;
    vkCmdBindVertexBuffers(cb, 0, 1, &vertexBuffer.buffer, &zeroOffset);
    vkCmdBindIndexBuffer(cb, indexBuffer.buffer, zeroOffset, VK_INDEX_TYPE_UINT16);
    vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(VkDeviceAddress), &shaderBuffers[frameIndex].deviceAddress);

    // Announce start of draw
    activeDrawCommandBuffer = cb;
    instanceCount = 0;
}

void drawMesh(MeshRef mesh, u32 instances) {
    assert(activeDrawCommandBuffer != VK_NULL_HANDLE);
    vkCmdDrawIndexed(activeDrawCommandBuffer, mesh.size, instances, mesh.ioff, 0, instanceCount);
    instanceCount += instances;
}

void endDraw() {
    assert(activeDrawCommandBuffer != VK_NULL_HANDLE);
    VkCommandBuffer cb = activeDrawCommandBuffer;

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

    // Announce end of draw
    activeDrawCommandBuffer = VK_NULL_HANDLE;
}
}