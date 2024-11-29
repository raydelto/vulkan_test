#include "VulkanRenderer.h"
#include <iostream>
#include <set>
#include <vector>
#include <array>

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
        setupDebugMessenger();
        createSurface();
        getPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        recordCommands();
        createSynchronization();
    }
    catch (const std::runtime_error& e)
    {
        printf("ERROR: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return 0;
}

void VulkanRenderer::draw()
{
    // -- GET NEXT IMAGE --

    // Wait for given fence to signal (open) from last draw before continuing
    vkWaitForFences(_mainDevice.logicalDevice, 1, &_drawFences[_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    // Manually reset (close) fences
    vkResetFences(_mainDevice.logicalDevice, 1, &_drawFences[_currentFrame]);

    uint32_t imageIndex; // Index of next image to be drawn to
    vkAcquireNextImageKHR(_mainDevice.logicalDevice, _swapchain, std::numeric_limits<uint64_t>::max(), _imageAvailable[_currentFrame], VK_NULL_HANDLE, &imageIndex);

    // -- SUBMIT COMMAND BUFFER TO RENDER --

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;                            // Number of semaphores to wait on
    submitInfo.pWaitSemaphores = &_imageAvailable[_currentFrame]; // List of semaphores to wait on
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.pWaitDstStageMask = waitStages;                      // Stages to check semaphores at
    submitInfo.commandBufferCount = 1;                              // Number of command buffers to submit
    submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];      // Command buffer to submit
    submitInfo.signalSemaphoreCount = 1;                            // Number of semaphores to signal
    submitInfo.pSignalSemaphores = &_renderFinished[_currentFrame]; // Semaphores to signal when command buffer finishes

    // Submit command buffer to queue
    if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _drawFences[_currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit Command Buffer to Queue!");
    }

    // -- PRESENT RENDERED IMAGE TO SCREEN --
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1; // Number of semaphores to wait on
    presentInfo.pWaitSemaphores = &_renderFinished[_currentFrame];
    ;                                        // Semaphores to wait on
    presentInfo.swapchainCount = 1;          // Number of swapchains to present to
    presentInfo.pSwapchains = &_swapchain;   // Swapchains to present images to
    presentInfo.pImageIndices = &imageIndex; // Index of images in swapchains to present

    // Present image
    if (vkQueuePresentKHR(_presentationQueue, &presentInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present Image!");
    }

    _currentFrame = (_currentFrame + 1) % MAX_FRAME_DRAWS;
}

void VulkanRenderer::cleanup()
{
    if (gEnableValidationLayers)
    {
        destroyDebugUtilsMessengerEXT();
    }

    for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
    {
        vkDestroySemaphore(_mainDevice.logicalDevice, _renderFinished[i], nullptr);
        vkDestroySemaphore(_mainDevice.logicalDevice, _imageAvailable[i], nullptr);
        vkDestroyFence(_mainDevice.logicalDevice, _drawFences[i], nullptr);
    }

    vkDestroyCommandPool(_mainDevice.logicalDevice, _graphicsCommandPool, nullptr);
    for (VkFramebuffer& framebuffer : _swapchainFramebuffers)
    {
        vkDestroyFramebuffer(_mainDevice.logicalDevice, framebuffer, nullptr);
    }
    vkDestroyPipeline(_mainDevice.logicalDevice, _graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(_mainDevice.logicalDevice, _pipelineLayout, nullptr);
    vkDestroyRenderPass(_mainDevice.logicalDevice, _renderPass, nullptr);
    for (SwapChainImage& image : _swapchainImages)
    {
        vkDestroyImageView(_mainDevice.logicalDevice, image.imageView, nullptr);
    }

    // Clean up swap chain before cleaning up logical device
    vkDestroySwapchainKHR(_mainDevice.logicalDevice, _swapchain, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_mainDevice.logicalDevice, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{

    // Print the message
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE; // Returning false indicates Vulkan should continue.
}

bool VulkanRenderer::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : gValidationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

VkResult VulkanRenderer::createDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(_instance, pCreateInfo, nullptr, &_debugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanRenderer::destroyDebugUtilsMessengerEXT()
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(_instance, _debugMessenger, nullptr);
    }
}

void VulkanRenderer::setupDebugMessenger()
{
    if (!gEnableValidationLayers)
    {
        return;
    }
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional

    if (createDebugUtilsMessengerEXT(&createInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void VulkanRenderer::createInstance()
{
    if (gEnableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("validation layers requested, but not available!");
    }
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

    if (gEnableValidationLayers)
    {
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Check Instance extensions are supported.
    if (!checkInstanceExtensionSupport(&instanceExtensions))
    {
        throw std::runtime_error("VkInstance does not support required extensions.");
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const auto& layer : availableLayers)
    {
        std::cout << layer.layerName << std::endl;
    }

    if (gEnableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(gValidationLayers.size());
        createInfo.ppEnabledLayerNames = gValidationLayers.data();
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
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

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

    // Vector for queue creation information, and set for family indices.
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

    // Queues the logical device needs to create and info to do so.
    for (int queueFamilyIndex : queueFamilyIndices)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex; // The index of the family to create a queue from.
        queueCreateInfo.queueCount = 1;                      // Number of queues to create.
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority; // Vulkan needs to know how to handle multiple queues.

        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Information to create logical device (sometimes called "device").
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());  // Number of queue create infos.
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();                            // List of queue create infos so device can create required queues.
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()); // Number of enabled logical device extensions.
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();                      // List of enabled logical device extensions.

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
    vkGetDeviceQueue(_mainDevice.logicalDevice, indices.presentationFamily, 0, &_presentationQueue);
}

void VulkanRenderer::createSurface()
{
    // Create a surface for the window to render to.
    if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a surface for the window.");
    }
}

void VulkanRenderer::createSwapChain()
{
    // Get swap chain details so we can pick best settings.
    SwapChainDetails swapChainDetails = getSwapChainDetails(_mainDevice.physicalDevice);

    // Find optimal surface values for our swap chain.
    VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
    VkPresentModeKHR presentMode = chooseBestPresentationMode(swapChainDetails.presentationModes);
    VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

    // How many images are in the swap chain? Get 1 more than the minimum to allow triple buffering.
    uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

    // If maxImageCount is 0, then infinite images can be in swap chain.
    if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 && imageCount > swapChainDetails.surfaceCapabilities.maxImageCount)
    {
        imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
    }

    // Creation information for swap chain.
    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = _surface;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.clipped = VK_TRUE;

    // Get queue family indices
    QueueFamilyIndices indices = getQueueFamilies(_mainDevice.physicalDevice);
    uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentationFamily };

    // If the graphics and presentation families are different, then swap chain must let images be shared between families.
    if (indices.graphicsFamily != indices.presentationFamily)
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;     // Optional
        swapChainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    // If old swap chain been destroyed and this one replaces it, then link old one to quickly hand over responsibilities.
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create Swap Chain.
    VkResult result = vkCreateSwapchainKHR(_mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &_swapchain);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Swap Chain.");
    }

    _swapchainImageFormat = surfaceFormat.format;
    _swapchainExtent = extent;

    // Get swap chain images (first count, then values).
    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(_mainDevice.logicalDevice, _swapchain, &swapchainImageCount, nullptr);
    std::vector<VkImage> images(swapchainImageCount);
    vkGetSwapchainImagesKHR(_mainDevice.logicalDevice, _swapchain, &swapchainImageCount, images.data());

    for (VkImage image : images)
    {
        SwapChainImage swapChainImage = {};
        swapChainImage.image = image;
        swapChainImage.imageView = createImageView(image, _swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

        // Add to swapchain image list.
        _swapchainImages.push_back(swapChainImage);
    }
}

void VulkanRenderer::createRenderPass()
{

    // ATTACHMENTS
    // Describes the attachments used by the render pass
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = _swapchainImageFormat;                    // Format to use for attachment
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;                   // Number of samples to write for multisampling
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;              // Describes what to do with attachment before rendering
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;            // Describes what to do with attachment after rendering
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;   // Describes what to do with stencil before rendering
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Describes what to do with stencil after rendering

    // Framebuffer data will be stored as an image, but images can be given different data layouts
    // to give optimal use for certain operations

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;     // Image data layout before render pass starts
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Image data layout after render pass (to change to)

    // Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;                                    // Attachment index (always 0 if only 1 attachment)
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout to use during the subpass

    // Information about a particular subpass the render pass is using
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Pipeline type subpass is to be bound to
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef; // Reference to the color attachment in the attachment list

    // Need to determine when layout transitions occur using subpass dependencies
    std::array<VkSubpassDependency, 2> subpassDependencies;

    // Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    // Transition must happen after...
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;                    // Producer of the dependency
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Pipeline stage
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;           // Stage access mask (memory access)
    // Transition must happen before...
    subpassDependencies[0].dstSubpass = 0;                                                                             // Producer of the dependency
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;                               // Pipeline stage
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Stage access mask (memory access)
    subpassDependencies[0].dependencyFlags = 0;

    // Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    // Transition must happen after...
    subpassDependencies[1].srcSubpass = 0;                                                                             // Producer of the dependency
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;                               // Pipeline stage
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Stage access mask (memory access)
    // Transition must happen before...
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;                    // Producer of the dependency
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Pipeline stage
    subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;           // Stage access mask (memory access)
    subpassDependencies[1].dependencyFlags = 0;

    // Create info for render pass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassInfo.pDependencies = subpassDependencies.data();

    // Create render pass
    if (vkCreateRenderPass(_mainDevice.logicalDevice, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Render Pass.");
    }
}

void VulkanRenderer::createGraphicsPipeline()
{
    // Read in SPIR-V code of shaders
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    // Build Shader Modules to link to Graphics Pipeline
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // -- SHADER STAGE CREATION INFORMATION --
    // Vertex Stage creation information
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Shader Stage name
    vertShaderStageInfo.module = vertShaderModule;          // Shader module to be used by stage
    vertShaderStageInfo.pName = "main";                     // Entry point in to the shader

    // Fragment Stage creation information
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; // Shader Stage name
    fragShaderStageInfo.module = fragShaderModule;            // Shader module to be used by stage
    fragShaderStageInfo.pName = "main";                       // Entry point in to the shader

    // Put shader stage creation info in to array
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // -- VERTEX INPUT
    // How the vertex data for an object is defined in memory
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

    // -- INPUT ASSEMBLY --
    // How the vertices are assembled in to a draw call
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Primitive type to assemble vertices as
    inputAssembly.primitiveRestartEnable = VK_FALSE;              // Allow overriding of "strip" topology to start new primitives

    // -- VIEWPORT & SCISSOR --
    // Create a viewport info struct
    VkViewport viewport = {};
    viewport.x = 0.0f;                                             // x start coordinate
    viewport.y = 0.0f;                                             // y start coordinate
    viewport.width = static_cast<float>(_swapchainExtent.width);   // width of viewport
    viewport.height = static_cast<float>(_swapchainExtent.height); // height of viewport
    viewport.minDepth = 0.0f;                                      // min framebuffer depth
    viewport.maxDepth = 1.0f;                                      // max framebuffer depth

    // Create a scissor info struct
    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };           // Offset to use region from
    scissor.extent = _swapchainExtent; // Extent to describe region to use, starting at offset

    // Combine viewport and scissor to create a viewport state info struct
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // -- DYNAMIC STATES --
    // Dynamic states to enable
    // std::vector<VkDynamicState> dynamicStateEnables;
    // dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT); // Dynamic Viewport : Can resize in command buffer with vkCmdSetViewport(commandbuffer, 0, 1, &viewport);
    // dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR); // Dynamic Scissor : Can resize in command buffer with vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

    //// Dynamic State creation info
    // VkPipelineDynamicStateCreateInfo dynamicState = {};
    // dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    // dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    // dynamicState.pDynamicStates = dynamicStateEnables.data();

    // -- RASTERIZER --
    // How to rasterize the polygons
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;                 // Change if fragments beyond near/far planes are clipped (default) or clamped to plane
    rasterizer.rasterizerDiscardEnable = VK_FALSE;          // Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;          // How to handle filling points between vertices
    rasterizer.lineWidth = 1.0f;                            // How thick lines should be when drawn
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;            // Which face of a tri to cull
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Winding to determine which side is front
    rasterizer.depthBiasEnable = VK_FALSE;                  // Whether to add depth bias to fragments (good for stopping "shadow acne")

    // -- MULTISAMPLING --
    // Multisampling to reduce aliasing
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;               // Enable multisample shading or not
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // Number of samples to use per fragment

    // -- COLOR BLENDING --
    // How to handle blending colors

    // Blend Attachment State (how blending is handled)
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; // Colors to apply blending to
    colorBlendAttachment.blendEnable = VK_TRUE;                                                                                                      // Enable blending

    // Blending uses equation: (srcColorBlendFactor * new color) colorBlendOp (dstColorBlendFactor * old color)
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

    // Summarized: (VK_BLEND_FACTOR_SRC_ALPHA * new color) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old color)
    //             (new color alpha * new color) + ((1 - new color alpha) * old color)

    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // Summarized: (1 * new alpha) + (0 * old alpha) = new alpha

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE; // Alternative to calculations is to use logical operations
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // -- PIPELINE LAYOUT --
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;            // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    // Create Pipeline Layout
    if (vkCreatePipelineLayout(_mainDevice.logicalDevice, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Pipeline Layout.");
    }

    // -- DEPTH STENCIL TESTING --

    // -- GRAPHICS PIPELINE CREATION --
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;                       // Number of shader stages
    pipelineInfo.pStages = shaderStages;               // List of shader stages
    pipelineInfo.pVertexInputState = &vertexInputInfo; // All the fixed function pipeline states
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pDynamicState = nullptr; // No dynamic state
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = nullptr;        // Pointer to depth stencil state creation info
    pipelineInfo.layout = _pipelineLayout;            // Pipeline Layout pipeline should use
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Handle to existing pipeline to derive from
    pipelineInfo.basePipelineIndex = -1;              // Index of base pipeline handle in case of array used to derive from
    pipelineInfo.renderPass = _renderPass;            // Render Pass the pipeline is compatible with
    pipelineInfo.subpass = 0;                         // Subpass of render pass to use with

    // Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimization
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Existing pipeline to derive from...
    pipelineInfo.basePipelineIndex = -1;              // or index of pipeline being created to derive from (in case creating multiple at once)

    // Create Graphics Pipeline
    if (vkCreateGraphicsPipelines(_mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Graphics Pipeline.");
    }

    // Destroy Shader Modules, no longer needed after Pipeline created
    vkDestroyShaderModule(_mainDevice.logicalDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(_mainDevice.logicalDevice, vertShaderModule, nullptr);
}

void VulkanRenderer::createFramebuffers()
{
    // Resize framebuffer count to equal swap chain image count
    _swapchainFramebuffers.resize(_swapchainImages.size());

    // Create a framebuffer for each swap chain image
    for (size_t i = 0; i < _swapchainFramebuffers.size(); i++)
    {
        VkImageView attachments[] = {
            _swapchainImages[i].imageView };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;         // Render Pass layout the Framebuffer will be used with
        framebufferInfo.attachmentCount = 1;              // Number of attachments
        framebufferInfo.pAttachments = attachments;       // List of attachments (1:1 with Render Pass)
        framebufferInfo.width = _swapchainExtent.width;   // Framebuffer width
        framebufferInfo.height = _swapchainExtent.height; // Framebuffer height
        framebufferInfo.layers = 1;                       // Framebuffer layers

        // Create Framebuffer
        if (vkCreateFramebuffer(_mainDevice.logicalDevice, &framebufferInfo, nullptr, &_swapchainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a Framebuffer.");
        }
    }
}

void VulkanRenderer::createCommandPool()
{
    // Get the queue family indices for the chosen physical device.
    QueueFamilyIndices queueFamilyIndices = getQueueFamilies(_mainDevice.physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;    // Queue Family type that buffers from this command pool will use
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Allow individual command buffers to be reset

    // Create a Graphics Queue Family Command Pool
    if (vkCreateCommandPool(_mainDevice.logicalDevice, &poolInfo, nullptr, &_graphicsCommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a command pool.");
    }
}

void VulkanRenderer::createCommandBuffers()
{
    // Resize command buffer count to have one for each framebuffer
    _commandBuffers.resize(_swapchainFramebuffers.size());

    VkCommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = _graphicsCommandPool;      // Pool to allocate command buffers from
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Primary vs Secondary command buffer
    cbAllocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());

    // Allocate command buffers and place handles in array of buffers
    if (vkAllocateCommandBuffers(_mainDevice.logicalDevice, &cbAllocInfo, _commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Command Buffers.");
    }
}

void VulkanRenderer::createSynchronization()
{
    _imageAvailable.resize(MAX_FRAME_DRAWS);
    _renderFinished.resize(MAX_FRAME_DRAWS);
    _drawFences.resize(MAX_FRAME_DRAWS);

    // Semaphore creation information
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Fence creation information
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
    {
        if (vkCreateSemaphore(_mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &_imageAvailable[i]) != VK_SUCCESS ||
            vkCreateSemaphore(_mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &_renderFinished[i]) != VK_SUCCESS ||
            vkCreateFence(_mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &_drawFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a Semaphore and/or Fence!");
        }
    }
}

void VulkanRenderer::recordCommands()
{
    // Information about how to begin each command buffer
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Buffer can be resubmitted when it has already been submitted and is awaiting execution

    // Information about how to begin a render pass (only needed for graphical applications)
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _renderPass;             // Render Pass to begin
    renderPassInfo.renderArea.offset = { 0, 0 };           // Start point of render pass in pixels
    renderPassInfo.renderArea.extent = _swapchainExtent; // Size of region to run render pass on

    std::array<VkClearValue, 1> clearValues = { 0.6f, 0.65f, 0.4f, 1.0f };

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = clearValues.data();

    // Loop to record command buffers
    for (size_t i = 0; i < _commandBuffers.size(); i++)
    {
        // Set the target frame buffer
        renderPassInfo.framebuffer = _swapchainFramebuffers[i];

        // Start recording commands to command buffer!
        if (vkBeginCommandBuffer(_commandBuffers[i], &bufferBeginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to start recording a Command Buffer.");
        }

        // Begin the render pass
        vkCmdBeginRenderPass(_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind the graphics pipeline
        vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

        // Draw triangle
        vkCmdDraw(_commandBuffers[i], 3, 1, 0, 0);

        // End the render pass
        vkCmdEndRenderPass(_commandBuffers[i]);

        // Finish recording commands to the command buffer
        if (vkEndCommandBuffer(_commandBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to stop recording a Command Buffer.");
        }
    }
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

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    // Get device extension count
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    // If no extensions found, return failure
    if (extensionCount == 0)
    {
        return false;
    }

    // Populate list of extensions
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

    for (const char* deviceExtension : deviceExtensions)
    {
        for (const auto& extension : extensions)
        {
            if (strcmp(deviceExtension, extension.extensionName) == 0)
            {
                return true;
            }
        }
    }
    return false;
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
    // VkPhysicalDeviceProperties deviceProperties;
    // vkGetPhysicalDeviceProperties(device, &deviceProperties);

    //// Information about what the device can do (geo shader, tess shader, wide lines, etc).
    // VkPhysicalDeviceFeatures deviceFeatures;
    // vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
    QueueFamilyIndices indices = getQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainValid = false;
    if (extensionsSupported)
    {
        SwapChainDetails swapChainDetails = getSwapChainDetails(device);
        swapChainValid = !swapChainDetails.formats.empty() && !swapChainDetails.presentationModes.empty();
    }

    return indices.isValid() && extensionsSupported && swapChainValid;
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

        // Check if queue family supports presentation
        VkBool32 presentationSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentationSupport);
        // Check if queue is presentation type (can be both graphics and presentation)
        if (queueFamily.queueCount > 0 && presentationSupport)
        {
            indices.presentationFamily = i;
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

SwapChainDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device)
{
    SwapChainDetails swapChainDetails;

    // -- CAPABILITIES --
    // Get the surface capabilities for the given surface on the given physical device
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &swapChainDetails.surfaceCapabilities);

    // -- FORMATS --
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

    // If formats returned, get list of formats
    if (formatCount != 0)
    {
        swapChainDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, swapChainDetails.formats.data());
    }

    // -- PRESENTATION MODES --
    uint32_t presentationCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentationCount, nullptr);

    // If presentation modes returned, get list of presentation modes
    if (presentationCount != 0)
    {
        swapChainDetails.presentationModes.resize(presentationCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentationCount, swapChainDetails.presentationModes.data());
    }
    return swapChainDetails;
}

// Best format is subjective. Start with looking for SRGB and RGBA8
// Format: VK_FORMAT_R8G8B8A8_UNORM
// ColorSpace: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    // If only 1 format available and is undefined, then this means ALL formats are available
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    // If restricted, search for optimal format
    for (const auto& format : formats)
    {
        if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
            && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    // If can't find optimal format, then return first format that is available
    return formats[0];
}

VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
    // Look for mailbox presentation mode
    for (const auto& presentationMode : presentationModes)
    {
        if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return presentationMode;
        }
    }
    // If can't find, use FIFO as Vulkan spec says it must be present
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
    // If current extent is at numeric limits, then extent can vary. Otherwise, it is the size of the window.
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return surfaceCapabilities.currentExtent;
    }
    else
    {
        // If value can vary, need to set manually

        // Get window size
        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);

        // Create new extent using window size
        VkExtent2D newExtent = {};
        newExtent.width = static_cast<uint32_t>(width);
        newExtent.height = static_cast<uint32_t>(height);

        // Surface also defines max and min, so make sure within boundaries by clamping value.
        newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
        newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

        return newExtent;
    }
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    // Create image view info
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;                                // Image to create view for
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;             // Type of image
    viewInfo.format = format;                              // Format of image data
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // Allows remapping of rgba components to other rgba values
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // Subresources allow the view to view only a part of an image
    viewInfo.subresourceRange.aspectMask = aspectFlags; // Which aspect of image to view (e.g. COLOR_BIT for viewing colour)
    viewInfo.subresourceRange.baseMipLevel = 0;         // Start mipmap level to view from
    viewInfo.subresourceRange.levelCount = 1;           // Number of mipmap levels to view
    viewInfo.subresourceRange.baseArrayLayer = 0;       // Start array level to view from
    viewInfo.subresourceRange.layerCount = 1;           // Number of array levels to view

    // Create image view and return it
    VkImageView imageView;
    if (vkCreateImageView(_mainDevice.logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create an Image View.");
    }

    return imageView;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
    // Shader Module creation information
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();                                  // Size of code
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // Pointer to code (of uint32_t pointer type)

    // Create Shader Module
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(_mainDevice.logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a shader module.");
    }

    return shaderModule;
}
