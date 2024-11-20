#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include "Utilities.h"

class VulkanRenderer
{
public:
    VulkanRenderer();
    ~VulkanRenderer();
    int init(GLFWwindow* newWindow);
    void cleanup();

private:
    GLFWwindow* _window;

    // Vulkan components
    VkInstance _instance;
    struct
    {
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
    } _mainDevice;
    VkQueue _graphicsQueue;

    // Vulkan functions
    // - Create Functions
    void createInstance();
    void createLogicalDevice();

    // Get functions
    void getPhysicalDevice();

    // Support functions
    bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
    bool checkDeviceSuitable(VkPhysicalDevice device);

    // Getter functions
    QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
    
};

