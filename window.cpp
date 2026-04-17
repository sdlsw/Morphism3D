#include "window.h"

#include <iostream>

namespace g3d {
void Window::initWindowingSystem() {
	std::cerr << "initializing GLFW..." << std::endl;
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	_glfwInitialized = true;
}

Window::Window(std::string title, uint32_t initialWidth, uint32_t initialHeight) {
	if (!_glfwInitialized) {
		initWindowingSystem();
	}

	_glfwWindow = glfwCreateWindow(
		initialWidth,
		initialHeight,
		title.c_str(),
		nullptr, nullptr
	);

	if (_glfwWindow == nullptr) {
		throw std::runtime_error("could not create GLFW window!");
	}

	glfwSetWindowUserPointer(_glfwWindow, this);
	glfwSetFramebufferSizeCallback(_glfwWindow, frameBufferResizeCallback);
	_glfwWindowCount++;

	std::cerr << "successfully created GLFW window at 0x" << this << std::endl;
}

Window::~Window() {
	if (_glfwWindow != nullptr) {
		glfwDestroyWindow(_glfwWindow);
		_glfwWindowCount--;

		if (_glfwWindowCount == 0) {
			std::cerr << "last window destroyed, terminating GLFW" << std::endl;
			glfwTerminate();
			_glfwInitialized = false;
		}
	}
}

Window* Window::getWrapperPointer(GLFWwindow* window) {
	return reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
}

void Window::frameBufferResizeCallback(GLFWwindow* window, int width, int height) {
	Window::getWrapperPointer(window)->_windowResized = true;
}

void Window::pollEvents() {
	glfwPollEvents();
}

bool Window::shouldClose() {
	return glfwWindowShouldClose(_glfwWindow);
}

WindowSize Window::size() {
	int w, h;
	glfwGetFramebufferSize(_glfwWindow, &w, &h);
	WindowSize s { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
	return s;
}

vk::raii::SurfaceKHR Window::createSurface(vk::raii::Instance& instance) {
	VkSurfaceKHR surface;
	VkResult result = glfwCreateWindowSurface(*instance, _glfwWindow, nullptr, &surface);

	if (result != VK_SUCCESS) {
		std::cerr << "glfwCreateWindowSurface returned status code " << result << std::endl;
		throw std::runtime_error("failed to create window surface!");
	}

	vk::raii::SurfaceKHR raiiSurface { instance, surface };
	return std::move(raiiSurface);
}
}
