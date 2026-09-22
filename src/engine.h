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

namespace engine {

// Ensure that the result is `VK_SUCCESS`; raise an unrecoverable error when it's not
void success(VkResult result, std::string errorMessage = "No Message Provided") {
    if (result == VK_SUCCESS) return;
    err(errorMessage, string_VkResult(result));
}

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
    // glm::vec3 normal;
};

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

struct ShaderUniformData {
    glm::mat4 proj;
    glm::mat4 view;
    glm::mat4 model;
};

std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}}
};

std::vector<u16> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

/* global settings */
std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_UNIFIED_IMAGE_LAYOUTS_EXTENSION_NAME };
std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

struct Engine {
public:
    /* general */
    u32 framesInFlight;
    u32 frameIndex;
    u32 imageIndex;
    bool framebufferResized;

    /* glfw */
    GLFWwindow* window;

    /* vulkan */
    VkInstance instance;
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    u32 queueFamily;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VmaAllocator allocator;
    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    /* size = number of swapchain images */
    Vec<VkImage> swapchainImages;
    Vec<VkImageView> swapchainImageViews;
    Vec<VkSemaphore> renderCompleteSemaphores;
    VkImage depthImage;
    VkFormat depthImageFormat;
    VkImageView depthImageView;
    VmaAllocation depthImageAllocation;
    BufferRef vertexBuffer;
    /* size = number of frames in flight */
    Vec<BufferRef> shaderBuffers;
    Vec<VkCommandBuffer> commandBuffers;
    Vec<VkFence> fences;
    Vec<VkSemaphore> imageAcquiredSemaphores;
    VkCommandPool commandPool; // TODO Create one per thread
    Vec<ImageRef> textures;
    Vec<VkDescriptorImageInfo> textureDescriptors;
    VkSampler sampler;
    VkDescriptorSetLayout texturesDescriptorSetLayout;
    VkDescriptorSet texturesDescriptorSet;
    VkDescriptorPool descriptorPool;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    /* fps */
    std::chrono::steady_clock::time_point lastFpsTimestamp;
    u32 frames;

    void init(u32 framesInFlight) {
        this->framesInFlight = framesInFlight;
        u32 bufSize = vertices.size() * sizeof(Vertex) + indices.size() * sizeof(u16);
        initWindow();
        createInstance();
        createPhysicalDevice();
        createSurface();
        createDevice(physicalDevice); // TODO just make a global array of extensions to use
        initVma();
        createSwapchain();
        createDepthImage();
        createVertexBuffers(bufSize);
        createSyncStructures();
        createCommandBuffers();
        createSamplers();
        textures.push(loadImage("E:/Pictures/Kyaru/Chibi/tear.png"));
        textureDescriptors.push(VkDescriptorImageInfo{
            .sampler = sampler,
            .imageView = textures[0].view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        });
        createDescriptorSetsForTextures();
        createGraphicsPipeline();

        // get the vertices in
        memcpy(vertexBuffer.allocationInfo.pMappedData, vertices.data(), vertices.size() * sizeof(Vertex));
        memcpy(((char*)vertexBuffer.allocationInfo.pMappedData) + vertices.size() * sizeof(Vertex), indices.data(), indices.size() * sizeof(u16));
    }
    void cleanup() {
        // TODO
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    void run() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(device);
    }
private:
    /* Run */

    void drawFrame() {
        success(vkWaitForFences(device, 1, &fences[frameIndex], VK_TRUE, UINT64_MAX));
        {
            VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAcquiredSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);
            if (result == VK_ERROR_OUT_OF_DATE_KHR) { recreateSwapchain(); return; }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) { err("Could not acquire swapchain image", string_VkResult(result)); }
        }
        success(vkResetFences(device, 1, &fences[frameIndex]));

        {
            static auto startTime = std::chrono::high_resolution_clock::now();

            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

            ShaderUniformData shaderUniform{
                .proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (f32)swapchainExtent.height, 0.1f, 10.0f),
                .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::normalize(glm::vec3(0.0f, 0.5f, 1.0f))),
            };
            shaderUniform.proj[1][1] *= -1;
            memcpy(shaderBuffers[frameIndex].allocationInfo.pMappedData, &shaderUniform, sizeof(ShaderUniformData));
        }

        VkCommandBuffer cb = commandBuffers[frameIndex];
        VkResult r4 = vkResetCommandBuffer(cb, 0);
        VkCommandBufferBeginInfo cbBI{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        VkResult r5 = vkBeginCommandBuffer(cb, &cbBI);

            std::vector<VkImageMemoryBarrier2> outputBarriers{
                VkImageMemoryBarrier2{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .srcAccessMask = 0,
                    .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_GENERAL, // TODO VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
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
                    .newLayout = VK_IMAGE_LAYOUT_GENERAL, // TODO VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
                    .image = depthImage,
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
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL, // TODO VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = { .color{ 0.0f, 0.0f, 0.2f, 1.0f } }
            };
            VkRenderingAttachmentInfo depthAttachmentInfo{
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = depthImageView,
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL, // TODO VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
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

            VkViewport vp{
                .width = (f32)swapchainExtent.width,
                .height = (f32)swapchainExtent.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f
            };
            vkCmdSetViewport(cb, 0, 1, &vp);
            VkRect2D scissor{ .extent = swapchainExtent };
            vkCmdSetScissor(cb, 0, 1, &scissor);

            vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            VkDeviceSize vOffset = 0;
            vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &texturesDescriptorSet, 0, nullptr);
            vkCmdBindVertexBuffers(cb, 0, 1, &vertexBuffer.buffer, &vOffset);
            vkCmdBindIndexBuffer(cb, vertexBuffer.buffer, vertices.size() * sizeof(Vertex), VK_INDEX_TYPE_UINT16);
            vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &shaderBuffers[frameIndex].deviceAddress);
            vkCmdDrawIndexed(cb, indices.size(), 1, 0, 0, 0);
            vkCmdEndRendering(cb);

            VkImageMemoryBarrier2 barrierPresent{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask = 0,
                .oldLayout = VK_IMAGE_LAYOUT_GENERAL, // TODO VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
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
        {
            VkResult result = vkQueueSubmit2(graphicsQueue, 1, &submitInfo, fences[frameIndex]);
            if (result != VK_SUCCESS) { err("Could not submit queue", string_VkResult(result)); }
        }

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
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) { framebufferResized = true; }
            else if (result != VK_SUCCESS) { err("Could not present swap chain image", string_VkResult(result)); }
        }

        if (framebufferResized) {
            framebufferResized = false;
            recreateSwapchain();
        }
    }

    /* Init */

    // create `this->window`
    void initWindow() {
        lastFpsTimestamp = std::chrono::high_resolution_clock::now();
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(500, 500, "Vulkan", nullptr, nullptr);
        //glfwSetWindowUserPointer(window, this);
        //glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    // create `this->instance`
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

    // create `this->physicalDevice`
    void createPhysicalDevice() {
        u32 deviceCount;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> devices; devices.resize(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        if (devices.size() == 1) {
            physicalDevice = devices[0];
        } else {
            physicalDevice = selectPhysicalDevice(Slice<VkPhysicalDevice>::from(devices));
        }
    }

    // create `this->surface` and `this->surfaceCapabilities`
    void createSurface() {
        success(glfwCreateWindowSurface(instance, window, nullptr, &surface), "Could not create window surface");
        success(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities), "Could not get surface capabilities");
    }

    // create `this->device`
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

    // create `this->allocator`
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

    // create `this->swapchain`, `this->swapchainImageFormat`, `this->swapchainExtent`, `this->swapchainImages`, `this->swapchainImageViews`
    void createSwapchain() {
        VkExtent2D swapchainExtent = surfaceCapabilities.currentExtent;
        if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) { err("Wayland moment"); }

        VkFormat imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
        VkSwapchainCreateInfoKHR swapchainCI{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = surfaceCapabilities.minImageCount,
            .imageFormat = imageFormat,
            .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
            .imageExtent = swapchainExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .oldSwapchain = swapchain
        };
        VkResult result = vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain);
        if (result != VK_SUCCESS) { err("Could not create swapchain", string_VkResult(result)); }

        swapchainImageFormat = imageFormat;
        this->swapchainExtent = swapchainExtent;

        u32 imageCount;
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data);
        swapchainImageViews.resize(imageCount);

        for (u32 i = 0; i < imageCount; i++) {
            VkImageViewCreateInfo viewCI{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = swapchainImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = imageFormat,
                .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
            };
            VkResult result = vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]);
            if (result != VK_SUCCESS) { err("Could not create image view", string_VkResult(result)); }
        }
    }

    // create `this->depthImage`, `this->depthImageAllocation`, `this->depthImageView`
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
        success(vmaCreateImage(allocator, &depthImageCI, &allocCI, &depthImage, &depthImageAllocation, nullptr), "Could not create image (depth buffer)");

        VkImageViewCreateInfo depthViewCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = depthImage,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = depthFormat,
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 }
        };
        success(vkCreateImageView(device, &depthViewCI, nullptr, &depthImageView), "Could not create image view (depth buffer)");
    }

    // create `this->vertexBuffer`, `this->shaderBuffers`
    void createVertexBuffers(VkDeviceSize bufferSize) {
        VkBufferCreateInfo bufferCI{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = bufferSize,
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
        };

        VmaAllocationCreateInfo vBufferAllocCI{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        {
            VkResult result = vmaCreateBuffer(allocator, &bufferCI, &vBufferAllocCI, &vertexBuffer.buffer, &vertexBuffer.allocation, &vertexBuffer.allocationInfo);
            if (result != VK_SUCCESS) { err("Could not allocate buffer", string_VkResult(result)); }
        }

        shaderBuffers.resize(framesInFlight);
        for (u32 i = 0; i < framesInFlight; i++) {
            VkBufferCreateInfo uBufferCI{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = sizeof(ShaderUniformData),
                .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            };
            VmaAllocationCreateInfo uBufferAllocCI{
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO
            };
            {
                VkResult result = vmaCreateBuffer(allocator, &uBufferCI, &uBufferAllocCI, &shaderBuffers[i].buffer, &shaderBuffers[i].allocation, &shaderBuffers[i].allocationInfo);
                if (result != VK_SUCCESS) { err("Could not allocate buffer", string_VkResult(result), "when allocating buffer #", i, "out of", framesInFlight); }
            }
            VkBufferDeviceAddressInfo uBufferBdaInfo{
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = shaderBuffers[i].buffer
            };
            shaderBuffers[i].deviceAddress = vkGetBufferDeviceAddress(device, &uBufferBdaInfo);
        }
    }

    // create `this->fences`, this->imageAcquiredSemaphores`, `this->renderCompleteSemaphores`
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
        renderCompleteSemaphores.resize(swapchainImages.size);
        for (VkSemaphore& semaphore : renderCompleteSemaphores) {
            VkResult result = vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore);
            if (result != VK_SUCCESS) { err("Could not create semaphore", string_VkResult(result)); }
        }
    }

    // create `this->commandPool`, `this->commandBuffers`
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
            VkResult result = vkAllocateCommandBuffers(device, &cbAllocCI, commandBuffers.data);
            if (result != VK_SUCCESS) { err("Could not create command buffers"); }
        }
    }

    // create `this->sampler`
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
    // create `this->texturesDescriptorSetLayout`, `this->descriptorPool`, `this->texturesDescriptorSet`
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
            .descriptorCount = textures.size,
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
            .descriptorCount = textures.size,
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

        u32 variableDescCount{ textures.size };
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
        {
            VkResult result = vkAllocateDescriptorSets(device, &texDescSetAlloc, &texturesDescriptorSet);
            if (result != VK_SUCCESS) { err("Cannot create descriptor set for textures", string_VkResult(result)); }
        }

        VkWriteDescriptorSet writeDescSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = texturesDescriptorSet,
            .dstBinding = 0,
            .descriptorCount = textureDescriptors.size,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = textureDescriptors.data
        };
        vkUpdateDescriptorSets(device, 1, &writeDescSet, 0, nullptr);
    }
    
    // create `this->pipeline`, `this->pipelineLayout`
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
        {
            VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout);
            if (result != VK_SUCCESS) { err("Could not create pipeline layout", string_VkResult(result)); }
        }
        // Describe the `struct Vertex`
        VkVertexInputBindingDescription vertexBinding{
             .binding = 0,
             .stride = sizeof(Vertex),
             .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
        std::vector<VkVertexInputAttributeDescription> vertexAttributes{
            {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos) },
            {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv) },
            //{.location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal) }
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
        {
            VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline);
            if (result != VK_SUCCESS) { err("Could not create graphics pipeline", string_VkResult(result)); }
        }
    }

    /* Helpers */

    VkPhysicalDevice selectPhysicalDevice(Slice<VkPhysicalDevice> devices) {
        for (u32 i = 0; i < devices.size; i++) {
            VkPhysicalDeviceProperties2 p;
            vkGetPhysicalDeviceProperties2(devices[i], &p);
            std::cout << i << ": " << p.properties.deviceName << std::endl;
        }
        std::cout << "Which to use? ";
        u32 choice;
        std::cin >> choice;
        return devices[choice];
    }

    void loadMeshObj(const char* path) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        {
            bool result = tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, path);
            if (!result) { err("Could not load obj"); }
        }

        const VkDeviceSize indexCount{ shapes[0].mesh.indices.size() };
        std::vector<Vertex> vertices{};
        std::vector<u16> indices{};

        for (auto& index : shapes[0].mesh.indices) {
            Vertex v{
                .pos = { attrib.vertices[index.vertex_index * 3], -attrib.vertices[index.vertex_index * 3 + 1], attrib.vertices[index.vertex_index * 3 + 2] },
                .uv = { attrib.texcoords[index.texcoord_index * 2], 1.0 - attrib.texcoords[index.texcoord_index * 2 + 1] },
                //.normal = { attrib.normals[index.normal_index * 3], -attrib.normals[index.normal_index * 3 + 1], attrib.normals[index.normal_index * 3 + 2] }
            };
            vertices.push_back(v);
            indices.push_back(indices.size());
        }
    }

    ImageRef loadImage(const char* path) {
        i32 texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (stbi_failure_reason()) {
            err("stbi", stbi_failure_reason());
        }

        VkDeviceSize imageSize = (u64)texWidth * (u64)texHeight * 4; // 4 bytes per pixel in VK_FORMAT_R8G8B8A8_SRGB

        ImageRef tex;
        createTextureImage(tex, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_HOST_TRANSFER_BIT);

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
                .width = (u32) texWidth,
                .height = (u32) texHeight,
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


        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        //std::cout << "start copy" << std::endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));


        vkCopyMemoryToImage(device, &memImgCopyInfo);


        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        //std::cout << "returning" << std::endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        
        stbi_image_free(pixels);
        return tex;
    }

    void createTextureImage(ImageRef& image, u32 width, u32 height, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT) {
        u32 mipLevels = 1;
        VkImageCreateInfo imageCI{
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
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // VK_IMAGE_LAYOUT_GENERAL
        };
        {
            VmaAllocationCreateInfo imageAllocCI{ .usage = VMA_MEMORY_USAGE_AUTO };
            VkResult result = vmaCreateImage(allocator, &imageCI, &imageAllocCI, &image.image, &image.allocation, nullptr);
            if (result != VK_SUCCESS) { err("Could not create image", string_VkResult(result)); }
        }

        VkImageViewCreateInfo imageViewCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = imageCI.format,
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = mipLevels, .layerCount = 1 }
        };
        {
            VkResult result = vkCreateImageView(device, &imageViewCI, nullptr, &image.view);
            if (result != VK_SUCCESS) { err("Could not create image view", string_VkResult(result)); }
        }
    }

    std::vector<u8> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) { err("Could not open file", filename); }

        size_t fileSize = (size_t)file.tellg();
        std::vector<u8> buffer(fileSize);

        file.seekg(0);
        file.read((char*) buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    bool isWindowMinimized() {
        i32 width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        return width == 0 || height == 0;
    }
    
    void recreateSwapchain() {
        while(isWindowMinimized()) { glfwWaitEvents(); }
        success(vkDeviceWaitIdle(device));
        success(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities), "Could not get surface capabilities");
        // `swapchainImages` are owned by `swapchain`, not `Engine`, so don't destroy them here
        for (u32 i = 0; i < swapchainImageViews.size; i++) {
            vkDestroyImageView(device, swapchainImageViews[i], nullptr);
        }
        vmaDestroyImage(allocator, depthImage, depthImageAllocation);
        vkDestroyImageView(device, depthImageView, nullptr);
        VkSwapchainKHR oldSwapchain = swapchain;
        createSwapchain();
        createDepthImage();
        vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
        for (VkSemaphore& semaphore : renderCompleteSemaphores) {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
        renderCompleteSemaphores.resize(swapchainImages.size);
        VkSemaphoreCreateInfo semaphoreCI{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        for (VkSemaphore& semaphore : renderCompleteSemaphores) {
            success(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
        }
    }
};
}