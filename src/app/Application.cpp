#include "app/Application.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <set>
#include <cstring>
#include <algorithm>

void Application::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void Application::initWindow() {
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Watermelon Refactored", nullptr, nullptr);
    if (!window) throw std::runtime_error("glfwCreateWindow failed");
}

void Application::initVulkan() {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createDepthResources();
    createFramebuffers();
    createCommandPool();
    createSphereGeometry();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSets();
    createGraphicsPipeline();
    createCommandBuffers();
    
    eventsManager.init(device, static_cast<uint32_t>(swapImages.size()));
}

void Application::createInstance() {
    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName   = "Watermelon";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "None";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_2;

    uint32_t glfwExtCount = 0;
    const char** glfwExt = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    std::vector<const char*> exts(glfwExt, glfwExt + glfwExtCount);

    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo        = &appInfo;
    ci.enabledExtensionCount   = static_cast<uint32_t>(exts.size());
    ci.ppEnabledExtensionNames = exts.data();

    if (vkCreateInstance(&ci, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("vkCreateInstance");
}

void Application::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("glfwCreateWindowSurface");
}

QueueFamilyIndices Application::findQueueFamilies(VkPhysicalDevice dev) {
    QueueFamilyIndices idx;
    uint32_t n = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &n, nullptr);
    std::vector<VkQueueFamilyProperties> props(n);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &n, props.data());

    for (uint32_t i = 0; i < n; ++i) {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) idx.graphics = i;
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &present);
        if (present) idx.present = i;
        if (idx.complete()) break;
    }
    return idx;
}

bool Application::checkDeviceExtensionSupport(VkPhysicalDevice dev) {
    uint32_t n = 0;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &n, nullptr);
    std::vector<VkExtensionProperties> exts(n);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &n, exts.data());
    for (auto& e : exts)
        if (strcmp(e.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) return true;
    return false;
}

SwapSupport Application::querySwapSupport(VkPhysicalDevice dev) {
    SwapSupport s;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &s.caps);
    uint32_t n = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &n, nullptr);
    s.formats.resize(n);
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &n, s.formats.data());
    n = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &n, nullptr);
    s.presentModes.resize(n);
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &n, s.presentModes.data());
    return s;
}

void Application::pickPhysicalDevice() {
    uint32_t n = 0;
    vkEnumeratePhysicalDevices(instance, &n, nullptr);
    if (n == 0) throw std::runtime_error("No Vulkan GPU");
    std::vector<VkPhysicalDevice> devs(n);
    vkEnumeratePhysicalDevices(instance, &n, devs.data());

    for (auto d : devs) {
        if (!findQueueFamilies(d).complete()) continue;
        if (!checkDeviceExtensionSupport(d))  continue;
        auto s = querySwapSupport(d);
        if (s.formats.empty() || s.presentModes.empty()) continue;
        physicalDevice = d;
        return;
    }
    throw std::runtime_error("No suitable GPU");
}

void Application::createLogicalDevice() {
    auto idx = findQueueFamilies(physicalDevice);
    graphicsFamily = idx.graphics.value();
    presentFamily  = idx.present.value();

    std::set<uint32_t> unique = { graphicsFamily, presentFamily };
    std::vector<VkDeviceQueueCreateInfo> qcis;
    float prio = 1.0f;
    for (uint32_t f : unique) {
        VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        q.queueFamilyIndex = f;
        q.queueCount       = 1;
        q.pQueuePriorities = &prio;
        qcis.push_back(q);
    }

    VkPhysicalDeviceFeatures features{};
    const char* exts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo ci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    ci.queueCreateInfoCount    = static_cast<uint32_t>(qcis.size());
    ci.pQueueCreateInfos       = qcis.data();
    ci.pEnabledFeatures        = &features;
    ci.enabledExtensionCount   = 1;
    ci.ppEnabledExtensionNames = exts;

    if (vkCreateDevice(physicalDevice, &ci, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("vkCreateDevice");

    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentFamily,  0, &presentQueue);
}

VkSurfaceFormatKHR Application::chooseFormat(const std::vector<VkSurfaceFormatKHR>& fmts) {
    for (auto& f : fmts)
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    return fmts[0];
}

VkPresentModeKHR Application::choosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
    for (auto m : modes) if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::chooseExtent(const VkSurfaceCapabilitiesKHR& caps) {
    if (caps.currentExtent.width != UINT32_MAX) return caps.currentExtent;
    VkExtent2D e{ WIDTH, HEIGHT };
    e.width  = std::clamp(e.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
    e.height = std::clamp(e.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return e;
}

void Application::createSwapchain() {
    auto sup = querySwapSupport(physicalDevice);
    auto fmt  = chooseFormat(sup.formats);
    auto mode = choosePresentMode(sup.presentModes);
    auto ext  = chooseExtent(sup.caps);

    swapFormat = fmt.format;
    swapExtent = ext;

    uint32_t imageCount = sup.caps.minImageCount + 1;
    if (sup.caps.maxImageCount > 0 && imageCount > sup.caps.maxImageCount)
        imageCount = sup.caps.maxImageCount;

    VkSwapchainCreateInfoKHR ci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    ci.surface          = surface;
    ci.minImageCount    = imageCount;
    ci.imageFormat      = fmt.format;
    ci.imageColorSpace  = fmt.colorSpace;
    ci.imageExtent      = ext;
    ci.imageArrayLayers = 1;
    ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t famIdx[] = { graphicsFamily, presentFamily };
    if (graphicsFamily != presentFamily) {
        ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices   = famIdx;
    } else {
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    ci.preTransform   = sup.caps.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode    = mode;
    ci.clipped        = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &ci, nullptr, &swapchain) != VK_SUCCESS)
        throw std::runtime_error("vkCreateSwapchainKHR");

    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapImages.data());
}

VkImageView Application::createImageView(VkImage img, VkFormat fmt, VkImageAspectFlags aspect) {
    VkImageViewCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    ci.image    = img;
    ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ci.format   = fmt;
    ci.subresourceRange.aspectMask     = aspect;
    ci.subresourceRange.baseMipLevel   = 0;
    ci.subresourceRange.levelCount     = 1;
    ci.subresourceRange.baseArrayLayer = 0;
    ci.subresourceRange.layerCount     = 1;
    VkImageView view;
    if (vkCreateImageView(device, &ci, nullptr, &view) != VK_SUCCESS)
        throw std::runtime_error("vkCreateImageView");
    return view;
}

void Application::createImageViews() {
    swapViews.resize(swapImages.size());
    for (size_t i = 0; i < swapImages.size(); ++i)
        swapViews[i] = createImageView(swapImages[i], swapFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Application::createRenderPass() {
    VkAttachmentDescription color{};
    color.format         = swapFormat;
    color.samples        = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth{};
    depth.format         = findDepthFormat();
    depth.samples        = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depthRef{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription sub{};
    sub.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount    = 1;
    sub.pColorAttachments       = &colorRef;
    sub.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> atts = { color, depth };

    VkRenderPassCreateInfo ci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    ci.attachmentCount = static_cast<uint32_t>(atts.size());
    ci.pAttachments    = atts.data();
    ci.subpassCount    = 1;
    ci.pSubpasses      = &sub;
    ci.dependencyCount = 1;
    ci.pDependencies   = &dep;

    if (vkCreateRenderPass(device, &ci, nullptr, &renderPass) != VK_SUCCESS)
        throw std::runtime_error("vkCreateRenderPass");
}

VkFormat Application::findDepthFormat() {
    VkFormat candidates[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
    for (auto f : candidates) {
        VkFormatProperties p;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, f, &p);
        if (p.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) return f;
    }
    throw std::runtime_error("no depth format");
}

void Application::createImage(uint32_t w, uint32_t h, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memFlags, VkImage& image, VkDeviceMemory& mem) {
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.imageType     = VK_IMAGE_TYPE_2D;
    ci.extent        = { w, h, 1 };
    ci.mipLevels     = 1;
    ci.arrayLayers   = 1;
    ci.format        = format;
    ci.tiling        = tiling;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ci.usage         = usage;
    ci.samples       = VK_SAMPLE_COUNT_1_BIT;
    ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &ci, nullptr, &image) != VK_SUCCESS)
        throw std::runtime_error("vkCreateImage");

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device, image, &req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize  = req.size;
    ai.memoryTypeIndex = findMemoryType(req.memoryTypeBits, memFlags);
    if (vkAllocateMemory(device, &ai, nullptr, &mem) != VK_SUCCESS)
        throw std::runtime_error("vkAllocateMemory(image)");
    vkBindImageMemory(device, image, mem, 0);
}

void Application::createDepthResources() {
    VkFormat fmt = findDepthFormat();
    createImage(swapExtent.width, swapExtent.height, fmt, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthMemory);
    depthView = createImageView(depthImage, fmt, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Application::createFramebuffers() {
    framebuffers.resize(swapViews.size());
    for (size_t i = 0; i < swapViews.size(); ++i) {
        std::array<VkImageView, 2> atts = { swapViews[i], depthView };
        VkFramebufferCreateInfo ci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        ci.renderPass      = renderPass;
        ci.attachmentCount = static_cast<uint32_t>(atts.size());
        ci.pAttachments    = atts.data();
        ci.width           = swapExtent.width;
        ci.height          = swapExtent.height;
        ci.layers          = 1;
        if (vkCreateFramebuffer(device, &ci, nullptr, &framebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("vkCreateFramebuffer");
    }
}

void Application::createCommandPool() {
    VkCommandPoolCreateInfo ci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = graphicsFamily;
    if (vkCreateCommandPool(device, &ci, nullptr, &cmdPool) != VK_SUCCESS)
        throw std::runtime_error("vkCreateCommandPool");
}

void Application::createSphereGeometry() {
    const float radius = 1.0f;
    const int stacks = 64;
    const int slices = 64;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (int i = 0; i <= stacks; ++i) {
        float v   = float(i) / stacks;
        float phi = v * glm::pi<float>();
        for (int j = 0; j <= slices; ++j) {
            float u     = float(j) / slices;
            float theta = u * glm::two_pi<float>();
            Vertex vert{};
            vert.pos = glm::vec3(
                radius * std::sin(phi) * std::cos(theta),
                radius * std::cos(phi),
                radius * std::sin(phi) * std::sin(theta)
            );
            vert.normal = glm::normalize(vert.pos);
            vert.uv     = glm::vec2(u, 1.0f - v);
            vertices.push_back(vert);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            uint32_t a = uint32_t(i * (slices + 1) + j);
            uint32_t b = a + uint32_t(slices + 1);
            indices.push_back(a); indices.push_back(b); indices.push_back(a + 1);
            indices.push_back(a + 1); indices.push_back(b); indices.push_back(b + 1);
        }
    }
    indexCount = uint32_t(indices.size());

    VkDeviceSize vSize = sizeof(Vertex) * vertices.size();
    VkBuffer vStaging; VkDeviceMemory vStagingMem;
    createBuffer(vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vStaging, vStagingMem);
    void* vData; vkMapMemory(device, vStagingMem, 0, vSize, 0, &vData); memcpy(vData, vertices.data(), size_t(vSize)); vkUnmapMemory(device, vStagingMem);
    createBuffer(vSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    copyBuffer(vStaging, vertexBuffer, vSize);
    vkDestroyBuffer(device, vStaging, nullptr); vkFreeMemory(device, vStagingMem, nullptr);

    VkDeviceSize iSize = sizeof(uint32_t) * indices.size();
    VkBuffer iStaging; VkDeviceMemory iStagingMem;
    createBuffer(iSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, iStaging, iStagingMem);
    void* iData; vkMapMemory(device, iStagingMem, 0, iSize, 0, &iData); memcpy(iData, indices.data(), size_t(iSize)); vkUnmapMemory(device, iStagingMem);
    createBuffer(iSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    copyBuffer(iStaging, indexBuffer, iSize);
    vkDestroyBuffer(device, iStaging, nullptr); vkFreeMemory(device, iStagingMem, nullptr);
}

uint32_t Application::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mem;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &mem);
    for (uint32_t i = 0; i < mem.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) && (mem.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    throw std::runtime_error("findMemoryType failed");
}

void Application::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProps, VkBuffer& buf, VkDeviceMemory& mem) {
    VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    ci.size = size; ci.usage = usage; ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &ci, nullptr, &buf) != VK_SUCCESS) throw std::runtime_error("vkCreateBuffer");
    VkMemoryRequirements req; vkGetBufferMemoryRequirements(device, buf, &req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = req.size; ai.memoryTypeIndex = findMemoryType(req.memoryTypeBits, memProps);
    if (vkAllocateMemory(device, &ai, nullptr, &mem) != VK_SUCCESS) throw std::runtime_error("vkAllocateMemory");
    vkBindBufferMemory(device, buf, mem, 0);
}

void Application::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pci.queueFamilyIndex = graphicsFamily; pci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    VkCommandPool pool; vkCreateCommandPool(device, &pci, nullptr, &pool);
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = pool; ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; ai.commandBufferCount = 1;
    VkCommandBuffer cmd; vkAllocateCommandBuffers(device, &ai, &cmd);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);
    VkBufferCopy region{ 0, 0, size }; vkCmdCopyBuffer(cmd, src, dst, 1, &region);
    vkEndCommandBuffer(cmd);
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO}; si.commandBufferCount = 1; si.pCommandBuffers = &cmd;
    vkQueueSubmit(graphicsQueue, 1, &si, VK_NULL_HANDLE); vkQueueWaitIdle(graphicsQueue);
    vkDestroyCommandPool(device, pool, nullptr);
}

void Application::createVertexBuffer() {}
void Application::createIndexBuffer() {}

void Application::createUniformBuffers() {
    uniformBuffers.resize(VulkanEventsManager::MAX_FRAMES_IN_FLIGHT);
    uniformMemory.resize(VulkanEventsManager::MAX_FRAMES_IN_FLIGHT);
    uniformMapped.resize(VulkanEventsManager::MAX_FRAMES_IN_FLIGHT);
    lightBuffers.resize(VulkanEventsManager::MAX_FRAMES_IN_FLIGHT);
    lightMemory.resize(VulkanEventsManager::MAX_FRAMES_IN_FLIGHT);
    lightMapped.resize(VulkanEventsManager::MAX_FRAMES_IN_FLIGHT);

    VkDeviceSize uboSize = sizeof(UBO); VkDeviceSize ligSize = 256;
    for (int i = 0; i < VulkanEventsManager::MAX_FRAMES_IN_FLIGHT; ++i) {
        createBuffer(uboSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformMemory[i]);
        vkMapMemory(device, uniformMemory[i], 0, uboSize, 0, &uniformMapped[i]);
        createBuffer(ligSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, lightBuffers[i], lightMemory[i]);
        vkMapMemory(device, lightMemory[i], 0, ligSize, 0, &lightMapped[i]);
    }
}

void Application::createDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 2> b{};
    b[0].binding = 0; b[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; b[0].descriptorCount = 1; b[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    b[1].binding = 1; b[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; b[1].descriptorCount = 1; b[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    ci.bindingCount = static_cast<uint32_t>(b.size()); ci.pBindings = b.data();
    if (vkCreateDescriptorSetLayout(device, &ci, nullptr, &descSetLayout) != VK_SUCCESS) throw std::runtime_error("vkCreateDescriptorSetLayout");
}

void Application::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> ps{};
    ps[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; ps[0].descriptorCount = VulkanEventsManager::MAX_FRAMES_IN_FLIGHT;
    ps[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; ps[1].descriptorCount = VulkanEventsManager::MAX_FRAMES_IN_FLIGHT;
    VkDescriptorPoolCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    ci.poolSizeCount = static_cast<uint32_t>(ps.size()); ci.pPoolSizes = ps.data(); ci.maxSets = VulkanEventsManager::MAX_FRAMES_IN_FLIGHT;
    if (vkCreateDescriptorPool(device, &ci, nullptr, &descPool) != VK_SUCCESS) throw std::runtime_error("vkCreateDescriptorPool");
}

void Application::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(VulkanEventsManager::MAX_FRAMES_IN_FLIGHT, descSetLayout);
    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    ai.descriptorPool = descPool; ai.descriptorSetCount = VulkanEventsManager::MAX_FRAMES_IN_FLIGHT; ai.pSetLayouts = layouts.data();
    descSets.resize(VulkanEventsManager::MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &ai, descSets.data()) != VK_SUCCESS) throw std::runtime_error("vkAllocateDescriptorSets");

    for (int i = 0; i < VulkanEventsManager::MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo ubi{ uniformBuffers[i], 0, sizeof(UBO) };
        VkDescriptorBufferInfo lbi{ lightBuffers[i],   0, sizeof(LightUBO) };
        std::array<VkWriteDescriptorSet, 2> w{};
        w[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; w[0].dstSet = descSets[i]; w[0].dstBinding = 0; w[0].descriptorCount = 1; w[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[0].pBufferInfo = &ubi;
        w[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; w[1].dstSet = descSets[i]; w[1].dstBinding = 1; w[1].descriptorCount = 1; w[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[1].pBufferInfo = &lbi;
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(w.size()), w.data(), 0, nullptr);
    }
}

static std::vector<char> readFile(const std::string& path) {
    std::ifstream f(path, std::ios::ate | std::ios::binary);
    if (!f.is_open()) throw std::runtime_error("Cannot open shader: " + path);
    size_t size = size_t(f.tellg()); std::vector<char> buf(size);
    f.seekg(0); f.read(buf.data(), size); return buf;
}

static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ci.codeSize = code.size(); ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule m;
    if (vkCreateShaderModule(device, &ci, nullptr, &m) != VK_SUCCESS) throw std::runtime_error("vkCreateShaderModule");
    return m;
}

void Application::createGraphicsPipeline() {
    auto vertCode = readFile(std::string(SHADER_DIR) + "/watermelon.vert.spv");
    auto fragCode = readFile(std::string(SHADER_DIR) + "/watermelon.frag.spv");

    VkShaderModule vertMod = createShaderModule(device, vertCode);
    VkShaderModule fragMod = createShaderModule(device, fragCode);

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT; stages[0].module = vertMod; stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; stages[1].module = fragMod; stages[1].pName = "main";

    auto bind = Vertex::binding(); auto attrs = Vertex::attributes();

    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = 1; vi.pVertexBindingDescriptions = &bind; vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size()); vi.pVertexAttributeDescriptions = attrs.data();

    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO}; ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO}; vp.viewportCount = 1; vp.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL; rs.cullMode = VK_CULL_MODE_BACK_BIT; rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO}; ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    ds.depthTestEnable = VK_TRUE; ds.depthWriteEnable = VK_TRUE; ds.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState cb{};
    cb.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; cb.blendEnable = VK_FALSE;
    VkPipelineColorBlendStateCreateInfo cbi{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO}; cbi.attachmentCount = 1; cbi.pAttachments = &cb;

    VkDynamicState dyn[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dsi{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO}; dsi.dynamicStateCount = 2; dsi.pDynamicStates = dyn;

    VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO}; pli.setLayoutCount = 1; pli.pSetLayouts = &descSetLayout;
    if (vkCreatePipelineLayout(device, &pli, nullptr, &pipelineLayout) != VK_SUCCESS) throw std::runtime_error("vkCreatePipelineLayout");

    VkGraphicsPipelineCreateInfo ci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    ci.stageCount = 2; ci.pStages = stages; ci.pVertexInputState = &vi; ci.pInputAssemblyState = &ia;
    ci.pViewportState = &vp; ci.pRasterizationState = &rs; ci.pMultisampleState = &ms; ci.pDepthStencilState = &ds;
    ci.pColorBlendState = &cbi; ci.pDynamicState = &dsi; ci.layout = pipelineLayout; ci.renderPass = renderPass; ci.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &ci, nullptr, &pipeline) != VK_SUCCESS)
        throw std::runtime_error("vkCreateGraphicsPipelines");

    vkDestroyShaderModule(device, vertMod, nullptr); vkDestroyShaderModule(device, fragMod, nullptr);
}

void Application::createCommandBuffers() {
    cmdBuffers.resize(swapImages.size());
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = cmdPool; ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; ai.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
    if (vkAllocateCommandBuffers(device, &ai, cmdBuffers.data()) != VK_SUCCESS) throw std::runtime_error("vkAllocateCommandBuffers");
}

void Application::updateUniforms(uint32_t frame) {
    UBO ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view  = glm::lookAt(glm::vec3(0.0f, 0.6f, 3.2f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj  = glm::perspective(glm::radians(50.0f), float(swapExtent.width) / float(swapExtent.height), 0.1f, 10.0f);
    ubo.proj[1][1] *= -1.0f;
    memcpy(uniformMapped[frame], &ubo, sizeof(ubo));

    LightUBO lb{};
    lb.lightDir   = glm::normalize(glm::vec3(1.0f, -1.2f, -0.6f));
    lb.lightColor = glm::vec3(1.0f, 0.97f, 0.85f);
    lb.viewPos    = glm::vec3(0.0f, 0.6f, 3.2f);
    memcpy(lightMapped[frame], &lb, sizeof(lb));
}

void Application::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) {
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cmd, &bi);
    std::array<VkClearValue, 2> clears{};
    clears[0].color = {{ 0.06f, 0.07f, 0.12f, 1.0f }}; clears[1].depthStencil = { 1.0f, 0 };
    VkRenderPassBeginInfo rbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rbi.renderPass = renderPass; rbi.framebuffer = framebuffers[imageIndex]; rbi.renderArea.offset = { 0, 0 }; rbi.renderArea.extent = swapExtent; rbi.clearValueCount = static_cast<uint32_t>(clears.size()); rbi.pClearValues = clears.data();
    vkCmdBeginRenderPass(cmd, &rbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    VkViewport vp{ 0.0f, 0.0f, float(swapExtent.width), float(swapExtent.height), 0.0f, 1.0f }; vkCmdSetViewport(cmd, 0, 1, &vp);
    VkRect2D sc{ {0, 0}, swapExtent }; vkCmdSetScissor(cmd, 0, 1, &sc);
    VkBuffer vb[] = { vertexBuffer }; VkDeviceSize off[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vb, off); vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSets[eventsManager.getCurrentFrame()], 0, nullptr);
    vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 0);
    vkCmdEndRenderPass(cmd); vkEndCommandBuffer(cmd);
}

void Application::drawFrame() {
    eventsManager.waitForFence(device);

    uint32_t imageIndex = 0;
    VkResult r = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, eventsManager.getImageAvailableSemaphore(), VK_NULL_HANDLE, &imageIndex);
    if (r == VK_ERROR_OUT_OF_DATE_KHR) return;

    updateUniforms(eventsManager.getCurrentFrame());

    eventsManager.resetFence(device);
    vkResetCommandBuffer(cmdBuffers[imageIndex], 0);
    recordCommandBuffer(cmdBuffers[imageIndex], imageIndex);

    VkSemaphore waitSems[] = { eventsManager.getImageAvailableSemaphore() };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore sigSems[] = { eventsManager.getRenderFinishedSemaphore(imageIndex) };

    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.waitSemaphoreCount = 1; si.pWaitSemaphores = waitSems; si.pWaitDstStageMask = waitStages;
    si.commandBufferCount = 1; si.pCommandBuffers = &cmdBuffers[imageIndex];
    si.signalSemaphoreCount = 1; si.pSignalSemaphores = sigSems;

    if (vkQueueSubmit(graphicsQueue, 1, &si, eventsManager.getCurrentFence()) != VK_SUCCESS)
        throw std::runtime_error("vkQueueSubmit");

    VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    pi.waitSemaphoreCount = 1; pi.pWaitSemaphores = sigSems;
    pi.swapchainCount = 1; pi.pSwapchains = &swapchain; pi.pImageIndices = &imageIndex;
    vkQueuePresentKHR(presentQueue, &pi);

    eventsManager.nextFrame();
}

void Application::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(device);
}

void Application::cleanup() {
    eventsManager.destroy(device);
    
    for (int i = 0; i < VulkanEventsManager::MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr); vkFreeMemory(device, uniformMemory[i], nullptr);
        vkDestroyBuffer(device, lightBuffers[i], nullptr); vkFreeMemory(device, lightMemory[i], nullptr);
    }
    vkDestroyDescriptorPool(device, descPool, nullptr); vkDestroyDescriptorSetLayout(device, descSetLayout, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr); vkFreeMemory(device, vertexBufferMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr); vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr); vkDestroyPipelineLayout(device, pipelineLayout, nullptr); vkDestroyRenderPass(device, renderPass, nullptr);
    for (auto fb : framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
    for (auto v : swapViews) vkDestroyImageView(device, v, nullptr);
    vkDestroyImageView(device, depthView, nullptr); vkDestroyImage(device, depthImage, nullptr); vkFreeMemory(device, depthMemory, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr); vkDestroyCommandPool(device, cmdPool, nullptr);
    vkDestroyDevice(device, nullptr); vkDestroySurfaceKHR(instance, surface, nullptr); vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window); glfwTerminate();
}
