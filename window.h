#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

namespace g3d {
struct WindowSize {
	uint32_t width;
	uint32_t height;
};

class Window {
private:
	static inline bool _glfwInitialized = false;
	static inline unsigned int _glfwWindowCount = 0;

	static Window* getWrapperPointer(GLFWwindow* window);
	static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);

	GLFWwindow* _glfwWindow = nullptr;
	bool _windowResized = false;
public:
	static void pollEvents();
	static void initWindowingSystem();
	
	Window() = delete;
	Window(const Window&) = delete;
	Window(std::string title, uint32_t initialWidth, uint32_t initialHeight);
	~Window();

	bool shouldClose();
	WindowSize size();

	vk::raii::SurfaceKHR createSurface(vk::raii::Instance& instance);
};
}
