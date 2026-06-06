#include "window.h"

#include <stb_image.h>

#include <iostream>

namespace g3d {
void Window::initWindowingSystem() {
	std::cerr << "initializing GLFW..." << std::endl;
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	_glfwInitialized = true;
}

Window::Window(
	EventRouter& eventRouter,
	std::string title,
	uint32_t initialWidth,
	uint32_t initialHeight
) {
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

	_eventRouter = &eventRouter;

	// Load window icons
	constexpr unsigned int numImages = 2;
	std::array<std::string, numImages> sizes { "16", "32" };
	std::vector<GLFWimage> images;
	for (const auto& size : sizes) {
		std::string iconPath = std::format("icon{}.png", size);

		GLFWimage image;
		image.pixels = stbi_load(iconPath.c_str(), &image.width, &image.height, 0, 4);
		if (image.pixels != nullptr) {
			images.push_back(image);
		} else {
			std::cerr << "[WINDOW][Warn] Could not load icon: " << iconPath << std::endl;
		}
	}
	glfwSetWindowIcon(_glfwWindow, images.size(), images.data());
	for (auto& image : images) stbi_image_free(image.pixels);

	glfwSetWindowUserPointer(_glfwWindow, this);
	glfwSetFramebufferSizeCallback(_glfwWindow, frameBufferResizeCallback);
	glfwSetKeyCallback(_glfwWindow, keyCallback);
	glfwSetMouseButtonCallback(_glfwWindow, mouseButtonCallback);
	glfwSetCursorPosCallback(_glfwWindow, mousePositionCallback);
	glfwSetScrollCallback(_glfwWindow, scrollCallback);
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

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	Window* wrapper = Window::getWrapperPointer(window);
	if (!wrapper->_discardKeyboardEvents) {
		KeyEvent e { window, key, scancode, action, mods };
		wrapper->eventRouter().routeEvent(e);
	}
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	Window* wrapper = Window::getWrapperPointer(window);
	if (!wrapper->_discardMouseEvents) {
		MouseButtonEvent e { window, button, action, mods };
		wrapper->eventRouter().routeEvent(e);
	}
}

void Window::mousePositionCallback(GLFWwindow* window, double xpos, double ypos) {
	Window* wrapper = Window::getWrapperPointer(window);
	if (!wrapper->_discardMouseEvents) {
		MousePositionEvent e { window, xpos, ypos };
		wrapper->eventRouter().routeEvent(e);
	}
}

void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	Window* wrapper = Window::getWrapperPointer(window);
	if (!wrapper->_discardMouseEvents) {
		ScrollEvent e { window, xoffset, yoffset };
		wrapper->eventRouter().routeEvent(e);
	}
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

void Window::pauseWhileMinimized() {
	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(_glfwWindow, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(_glfwWindow, &width, &height);
		glfwWaitEvents();
	}
}

void Window::title(const std::string& newTitle) {
	glfwSetWindowTitle(_glfwWindow, newTitle.c_str());
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

void Window::incCursorPosition(double xinc, double yinc) {
	double x;
	double y;

	glfwGetCursorPos(_glfwWindow, &x, &y);
	glfwSetCursorPos(_glfwWindow, x + xinc, y + yinc);
}

int Window::cursorMode() {
	return glfwGetInputMode(_glfwWindow, GLFW_CURSOR);
}

std::ostream& operator<<(std::ostream& out, const KeyEvent& e) {
	return out << e.key << ", " << e.action;
}

std::ostream& operator<<(std::ostream& out, const MouseButtonEvent& e) {
	return out << e.button << ", " << e.action;
}

std::ostream& operator<<(std::ostream& out, const MousePositionEvent& e) {
	return out << e.xpos << ", " << e.ypos;
}

std::ostream& operator<<(std::ostream& out, const ScrollEvent& e) {
	return out << e.xoffset << ", " << e.yoffset;
}
}
