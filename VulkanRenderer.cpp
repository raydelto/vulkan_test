#include "VulkanRenderer.h"
#include <iostream>

VulkanRenderer::VulkanRenderer()
{
}

VulkanRenderer::~VulkanRenderer()
{
}

int VulkanRenderer::init(GLFWwindow* newWindow)
{
    _window = newWindow;
    try
    {
        createInstance();
    }
    catch (const std::runtime_error& e)
    {
        printf("ERROR: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return 0;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    // Print the message
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE; // Returning false indicates Vulkan should continue.
}

void VulkanRenderer::createInstance()
{
    // Information about the application.
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;


    // Information about the vulkan instance.
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &appInfo;

    // Create a list to hold instance extensions.
    std::vector<const char*> instanceExtensions = std::vector<const char*>();
    
    // Set up extensions the instance will use.
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions; // Extensions passed as array of cstrings.

    // Get GLFW extensions.
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Add GLFW extensions to list of extensions.
    for (size_t i = 0; i < glfwExtensionCount; i++)
    {
        std::cout << "GLFW Extension: " << glfwExtensions[i] << std::endl;
        instanceExtensions.push_back(glfwExtensions[i]);
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();



    const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };


    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = validationLayers;

    // TODO: Set up validation layers the instance will use.
    //createInfo.enabledLayerCount = 0;
    //createInfo.ppEnabledExtensionNames = nullptr;



    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    debugCreateInfo.pUserData = nullptr; // Optional


    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

    // Create instance.
    VkResult result = vkCreateInstance(&createInfo, nullptr, &_instance);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Vulkan Instance.");
    }
}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
    // Get number of extensions.
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    return false;
}
