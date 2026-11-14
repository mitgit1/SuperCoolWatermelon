#include "core/VulkanEventsManager.h"
#include <stdexcept>

void VulkanEventsManager::init(VkDevice device, uint32_t swapchainImageCount) {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(swapchainImageCount);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create sync objects for a frame!");
        }
    }

    for (size_t i = 0; i < swapchainImageCount; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render finished semaphores!");
        }
    }
}

void VulkanEventsManager::destroy(VkDevice device) {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }
    for (auto semaphore : renderFinishedSemaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
}

void VulkanEventsManager::waitForFence(VkDevice device) {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
}

void VulkanEventsManager::resetFence(VkDevice device) {
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
}

VkSemaphore VulkanEventsManager::getImageAvailableSemaphore() const {
    return imageAvailableSemaphores[currentFrame];
}

VkSemaphore VulkanEventsManager::getRenderFinishedSemaphore(uint32_t imageIndex) const {
    return renderFinishedSemaphores[imageIndex];
}

VkFence VulkanEventsManager::getCurrentFence() const {
    return inFlightFences[currentFrame];
}
