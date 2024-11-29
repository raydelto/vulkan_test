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
    void draw();
    void cleanup();

private:
    GLFWwindow* _window;
    int _currentFrame = 0;

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
    VkDebugUtilsMessengerEXT _debugMessenger;


    // - Swap chain

    std::vector<SwapChainImage> _swapchainImages;
    std::vector<VkFramebuffer> _swapchainFramebuffers;
    std::vector<VkCommandBuffer> _commandBuffers;

    // Vulkan functions

    // - Validation
    bool checkValidationLayerSupport();
    void setupDebugMessenger();
    VkResult createDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo);
    void destroyDebugUtilsMessengerEXT();

    // - Pipeline
    VkPipeline _graphicsPipeline;
    VkPipelineLayout _pipelineLayout;
    VkRenderPass _renderPass;

    // - Pools
    VkCommandPool _graphicsCommandPool;


    // - Utility
    VkFormat _swapchainImageFormat;
    VkExtent2D _swapchainExtent;

    // Syncronization
    std::vector<VkSemaphore> _imageAvailable;
    std::vector<VkSemaphore> _renderFinished;
    std::vector<VkFence> _drawFences;

    // Vulkan functions
    // - Create Functions
    void createInstance();
    void createLogicalDevice();
    void createSurface();
    void createSwapChain(); 
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSynchronization();

    // - Record Functions
    void recordCommands();


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

