#pragma once

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Indices (locations) of queue families (if they exist at all)
struct QueueFamilyIndices
{
    int graphicsFamily = -1; // Location of graphics queue family
    int presentationFamily = -1; // Location of presentation queue family

    // Check if queue families are valid
    bool isValid()
    {
        return graphicsFamily >= 0 && presentationFamily >=0;
    }
};

// Swap chain details
struct SwapChainDetails
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities; // Surface properties, e.g. image size/extent
    std::vector<VkSurfaceFormatKHR> formats; // Surface image formats, e.g. RGBA and size of each colour
    std::vector<VkPresentModeKHR> presentationModes; // How images should be presented to the screen
};