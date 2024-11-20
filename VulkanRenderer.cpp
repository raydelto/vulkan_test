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
        getPhysicalDevice();
        createLogicalDevice();
    }
    catch (const std::runtime_error& e)
    {
        printf("ERROR: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return 0;
}

void VulkanRenderer::cleanup()
{
    vkDestroyDevice(_mainDevice.logicalDevice, nullptr);
    vkDestroyInstance(_instance, nullptr);
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

    // Check Instance extensions are supported.
    if (!checkInstanceExtensionSupport(&instanceExtensions))
    {
        throw std::runtime_error("VkInstance does not support required extensions.");
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

void VulkanRenderer::createLogicalDevice()
{
    // Get the queue family indices for the chosen physical device.
    QueueFamilyIndices indices = getQueueFamilies(_mainDevice.physicalDevice);

    // Queue the logical device needs to create and info to do so.
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily; // The index of the family to create a queue from.
    queueCreateInfo.queueCount = 1; // Number of queues to create.
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority; // Vulkan needs to know how to handle multiple queues.

    // Information to create logical device (sometimes called "device").
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1; // Number of queue create infos.
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo; // List of queue create infos so device can create required queues.
    deviceCreateInfo.enabledExtensionCount = 0; // Number of enabled logical device extensions.
    deviceCreateInfo.ppEnabledExtensionNames = nullptr; // List of enabled logical device extensions.

    deviceCreateInfo.pEnabledFeatures = nullptr; // Features the logical device will use.

    // Physical device features the logical device will use.
    VkPhysicalDeviceFeatures deviceFeatures = {};

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures; // Physical device features logical device will use.

    // Create the logical device for the given physical device.
    VkResult result = vkCreateDevice(_mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &_mainDevice.logicalDevice);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a logical device.");
    }

    // Queues are created at the same time as the device.
    // So we want to handle the queues.
    // From given logical device, of given queue family, of given queue index (0 since only one queue), place reference in _graphicsQueue.
    vkGetDeviceQueue(_mainDevice.logicalDevice, indices.graphicsFamily, 0, &_graphicsQueue);
}

void VulkanRenderer::getPhysicalDevice()
{
    // Enumerate Physical devices the vkInstance can access.
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

    // If no devices available, then none support Vulkan!
    if (deviceCount == 0)
    {
        throw std::runtime_error("Can't find GPUs that support Vulkan Instance.");
    }

    // Get list of physical devices.
    std::vector<VkPhysicalDevice> deviceList(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, deviceList.data());

    for (const auto& device : deviceList)
    {
        if (checkDeviceSuitable(device))
        {
            _mainDevice.physicalDevice = device;
            return;
        }
    }
    throw std::runtime_error("No suitable GPUs were found.");
}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
    // Need to get number of extensions to create array of correct size to hold extensions.
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    // Create a list of VkExtensionProperties using count.
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    // Check if given extensions are in list of available extensions.
    for (const char* extensionName : *checkExtensions)
    {
        bool hasExtension = false;
        for (const auto& extension : extensions)
        {
            if (strcmp(extensionName, extension.extensionName) == 0)
            {
                hasExtension = true;
                break;
            }
        }

        if (!hasExtension)
        {
            return false;
        }
    }

    return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
    //// Information about the device itself (ID, name, type, vendor, etc).
    //VkPhysicalDeviceProperties deviceProperties;
    //vkGetPhysicalDeviceProperties(device, &deviceProperties);

    //// Information about what the device can do (geo shader, tess shader, wide lines, etc).
    //VkPhysicalDeviceFeatures deviceFeatures;
    //vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    //return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
    QueueFamilyIndices indices = getQueueFamilies(device);

    return indices.isValid();
}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{

    QueueFamilyIndices indices;

    // Get all Queue Family Property info for the given device
    uint32_t queueFamilyCount = 0;

    if (device != VK_NULL_HANDLE)
    {
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    }
    else
    {
        // Handle the error appropriately
        throw std::runtime_error("Physical device is not initialized.");
    }
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

    // Go through each queue family and check if it has at least 1 of the required types of queue
    int i = 0;
    for (const auto& queueFamily : queueFamilyList)
    {
        // Check if queue family has at least 1 queue in that family (could have no queues)
        // Queue can be multiple types defined through bitfield. Need to bitwise AND with VK_QUEUE_*_BIT to check if has required type
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i; // If queue family is valid, then get index
        }
        i++;

        // Check if queue families are valid
        if (indices.isValid())
        {
            break;
        }
    }

    return indices;
}
