#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>

#include "VulkanRenderer.h"

GLFWwindow* window = nullptr;

void initWindow(std::string wName = "Test Window", const int width = 800, const int height = 600)
{
    // Initialize GLFW
    glfwInit();

    // Set GLFW to not work with OpenGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
}

int main()
{
    initWindow("Test Window", 800, 600);

    VulkanRenderer vulkanRenderer;
    if(vulkanRenderer.init(window) == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        vulkanRenderer.draw();
    }

    vulkanRenderer.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
	return 0;
}