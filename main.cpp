#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>
#include <cstdlib>

#include "VulkanRenderer.h"

GLFWwindow *window;
VulkanRenderer vulkanRenderer;

void initWindow(std::string wName = "Test Window", const int width = 800, const int height = 600)
{
	// Initialize GLFW
	glfwInit();

	// Set GLFW to NOT work with OpenGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}

int main()
{
	// Create Window
	initWindow("Test Window", 1366, 768);

	// Create Vulkan Renderer instance
	if (vulkanRenderer.init(window) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	float angle = 0.0f;
	float deltaTime = 0.0f;
	float lastTime = 0.0f;

	int helicopter = vulkanRenderer.createMeshModel("Models/uh60.obj");

	// Loop until closed
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		float now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		angle += 10.0f * deltaTime;
		if (angle >= 360.0f)
		{
			angle = 0.0f;
		}
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(-30.0f, -20.0f, -75.0f));
		glm::mat4 transformMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
		transformMatrix = translation * transformMatrix;
		vulkanRenderer.updateModel(helicopter, transformMatrix);
		vulkanRenderer.draw();
	}

	vulkanRenderer.cleanup();

	// Destroy GLFW window and stop GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	return EXIT_SUCCESS;
}