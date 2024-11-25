#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <algorithm>
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
    VkQueue _presentationQueue;
    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    std::vector<SwapChainImage> _swapchainImages;

    // - Utility
    VkFormat _swapchainImageFormat;
    VkExtent2D _swapchainExtent;

    // Vulkan functions
    // - Create Functions
    void createInstance();
    void createLogicalDevice();
    void createSurface();
    void createSwapChain();
    void createGraphicsPipeline();

    // Get functions
    void getPhysicalDevice();

    // Support functions
    // - Checker functions
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
    bool checkDeviceSuitable(VkPhysicalDevice device);

    // Getter functions
    QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
    SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);

    // Choose functions
    VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

    // Create functions
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    
};

