#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

class VulkanRenderer
{
public:
    VulkanRenderer();
    ~VulkanRenderer();
    int init(GLFWwindow* newWindow);

private:
    GLFWwindow* _window;

    // Vulkan components
    VkInstance _instance;

    // Vulkan functions
    void createInstance();
};

