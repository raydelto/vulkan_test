#pragma once

#include <fstream>
#include <vector>

#ifdef NDEBUG
const bool gEnableValidationLayers = false;
#else
const bool gEnableValidationLayers = true;
#endif

constexpr int MAX_FRAME_DRAWS = 2;

const std::vector<const char*> gValidationLayers = {
    "VK_LAYER_KHRONOS_validation", "VK_LAYER_NV_optimus"
};

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

struct SwapChainImage
{
    VkImage image;
    VkImageView imageView;
};

static std::vector<char> readFile(const std::string& filename)
{
    // Open stream from given file
    // std::ios::binary tells stream to read file as binary
    // std::ios::ate tells stream to start reading from end of file
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    // Check if file stream successfully opened
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    // Get current read position and use to resize file buffer
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> fileBuffer(fileSize);

    // Move read position (seek to) beginning of file
    file.seekg(0);

    // Read the file data into the buffer (stream "fileSize" in total)
    file.read(fileBuffer.data(), fileSize);

    // Close the file stream
    file.close();

    return fileBuffer;
}