#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

class VulkanEventsManager {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    void init(VkDevice device, uint32_t swapchainImageCount);
    void destroy(VkDevice device);

    void waitForFence(VkDevice device);
    void resetFence(VkDevice device);

    VkSemaphore getImageAvailableSemaphore() const;
    VkSemaphore getRenderFinishedSemaphore(uint32_t imageIndex) const;
    VkFence getCurrentFence() const;

    uint32_t getCurrentFrame() const { return currentFrame; }
    void nextFrame() { currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; }

private:
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
};
