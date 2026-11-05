#pragma once

#include "engine.h"

namespace gizmos {

u32 instanceCount = 0;

void startDraw() {
    VkCommandBuffer cb = engine::activeDrawCommandBuffer;
    assert(cb != VK_NULL_HANDLE);
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, engine::pipelineGizmos);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, engine::pipelineLayoutGizmos, 0, 1, &engine::texturesDescriptorSet, 0, nullptr);
    vkCmdPushConstants(cb, engine::pipelineLayoutGizmos, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(VkDeviceAddress), &engine::shaderBuffers[engine::frameIndex].deviceAddress);
    instanceCount = 0;
}

void drawRangeIndicator(glm::vec2 pos, f32 radius) {
    assert(engine::activeDrawCommandBuffer != VK_NULL_HANDLE);
    vkCmdDrawIndexed(engine::activeDrawCommandBuffer, 4, 1, 0, 0, instanceCount);
}

}