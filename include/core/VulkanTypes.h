#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <cstddef>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;

    static VkVertexInputBindingDescription binding() {
        return { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
    }
    static std::array<VkVertexInputAttributeDescription, 3> attributes() {
        std::array<VkVertexInputAttributeDescription, 3> a{};
        a[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) };
        a[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) };
        a[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, uv) };
        return a;
    }
};

struct UBO {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct LightUBO {
    glm::vec3 lightDir;    float _p0 = 0;
    glm::vec3 lightColor;  float _p1 = 0;
    glm::vec3 viewPos;     float _p2 = 0;
};
