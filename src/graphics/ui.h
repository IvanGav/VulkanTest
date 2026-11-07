#pragma once

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_vulkan.h"
#include "engine.h"
#include "../game/logic.h"

namespace ui {

bool captureMouse = false;
bool captureKeyboard = false;

void framebufferResizedCallback() {}

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

u32 placementMonkeySelectedIndex = U32_MAX;
void monkeyButton(u32 index, const char* name) {
    if (placementMonkeySelectedIndex == index) {
        ImGui::PushStyleColor(0, ImVec4(0.3, 1.0, 0.3, 1.0));
        if (ImGui::Button(name)) { placementMonkeySelectedIndex = U32_MAX; }
        ImGui::PopStyleColor();
    } else {
        if (ImGui::Button(name)) { placementMonkeySelectedIndex = index; }
    }
}

TID selectedMonkey = {};
void renderMonkeyWindow() {
    assert(selectedMonkey != TID{});
    ntower::Tower const* tower = towers->find(selectedMonkey);
    bool show = true;
    ImGui::Begin("Selected Tower Window", &show);
    ImGui::Text("%s", tower->proto->tower_name);
    ImGui::Text("Pops: %i", tower->pop_count);
    ImGui::End();
    // remove selected tower on delete
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        remove_tower(selectedMonkey);
        selectedMonkey = TID{};
    }
    if (!show) { selectedMonkey = TID{}; } // if window closed, deselect monkey
}

void renderBloonSpawnWindow(bool* show) {
    assert(*show == true);
    ImGui::Begin("Spawn Bloons", show);
    for (nbloon::Proto& proto : bloon_protos) {
        if (ImGui::Button(proto.name)) { bloons->add(nbloon::Bloon::spawn(&proto, &path)); }
    }
    ImGui::End();
}

bool showDemoWindow = false;
bool showBloonSpawnWindow = false;
void createUI() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    if (showDemoWindow) { ImGui::ShowDemoWindow(&showDemoWindow); }
    if (showBloonSpawnWindow) { renderBloonSpawnWindow(&showBloonSpawnWindow); }
    if (selectedMonkey != TID{}) { renderMonkeyWindow(); }
    // main ui window
    {
        ImGui::Begin("BTDC"); // ImGuiWindowFlags_;

        ImGui::Text("Bloons");
        ImGui::Checkbox("Demo Window", &showDemoWindow);
        ImGui::Checkbox("Spawn Bloons Window", &showBloonSpawnWindow);

        ImGui::Text("FPS: %.1f (%.3f ms/frame)", data::fps, 1000.0f / data::fps);
        ImGui::Text("UPS: %.1f (%.3f ms/update)", data::ups, 1000.0f / data::ups);

        ImGui::Text("Bloons: %i", bloons->list.size);
        ImGui::Text("Projs:  %i", projs->list.size);
        ImGui::Text("Towers: %i", towers->list.size);

        ImGui::SliderInt("Threads", (int*) &compute_thread_count, 1, 12);

        for (u32 i = 0; i < tower_protos.size; i++) {
            monkeyButton(i, tower_protos[i].tower_name);
        }

        ImGui::End();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        ImGui::SetWindowFocus(NULL);
    }
    ImGui::Render();

    ImGuiIO& io = ImGui::GetIO();
    captureMouse = io.WantCaptureMouse;
    captureKeyboard = io.WantCaptureKeyboard;
}

}