#pragma once

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_vulkan.h"
#include "engine.h"

namespace ui {

bool captureMouse = false;
bool captureKeyboard = false;

//static ImGui_ImplVulkanH_Window window;

void framebufferResizedCallback() {
    //ImGui_ImplVulkan_SetMinImageCount(engine::minImageCount); // why is it needed?
    //ImGui_ImplVulkanH_CreateOrResizeWindow(engine::instance, engine::physicalDevice, engine::device, &ui::window, engine::queueFamily, engine::allocator->GetAllocationCallbacks(), data::framebufferWidth, data::framebufferHeight, engine::minImageCount, 0);
}

// this must be called between `engine::startDraw` and `engine::endDraw`
void drawUI() {
    assert(engine::activeDrawCommandBuffer != VK_NULL_HANDLE);
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, engine::activeDrawCommandBuffer);
}

void cleanup() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void init() {
    f32 main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
     io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    style.FontSizeBase = 13.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    ImGui_ImplGlfw_InitForVulkan(engine::window, true);
    ImGui_ImplVulkan_InitInfo init_info = {
        .ApiVersion = VK_API_VERSION_1_4,
        .Instance = engine::instance,
        .PhysicalDevice = engine::physicalDevice,
        .Device = engine::device,
        .QueueFamily = engine::queueFamily,
        .Queue = engine::graphicsQueue,
        .DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE + IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE,
        .MinImageCount = engine::minImageCount,
        .ImageCount = engine::framesInFlight,
        .PipelineInfoMain {
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .PipelineRenderingCreateInfo = engine::renderingCI
        },
        .UseDynamicRendering = true,
        .Allocator = engine::allocator->GetAllocationCallbacks(),
        .CheckVkResultFn = engine::success,
    };
    ImGui_ImplVulkan_Init(&init_info);
    ImGui::PushFont(io.Fonts->AddFontFromFileTTF("./asset/JetBrainsMono-Regular.ttf"));
}

bool show_demo_window = false;
i32 counter = 0;
f32 f = 0.0f;
f32 col[3] = {};
void createUI() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (show_demo_window) { ImGui::ShowDemoWindow(&show_demo_window); }
    // main ui window
    {
        ImGui::Begin("BTDC"); // ImGuiWindowFlags_;

        ImGui::Text("Bloons");
        ImGui::Checkbox("Demo Window", &show_demo_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", col);

        if (ImGui::Button("Button")) { counter++; }
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / data::fps, data::fps);
        ImGui::End();
    }
    ImGui::Render();

    ImGuiIO& io = ImGui::GetIO();
    captureMouse = io.WantCaptureMouse;
    captureKeyboard = io.WantCaptureKeyboard;
}

}