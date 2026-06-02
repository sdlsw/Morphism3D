#pragma once
#include "global_defines.h"

#include "event.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

#include <iostream>
#include <unordered_map>
#include <string>

namespace g3d {
struct KeyEvent {
	GLFWwindow* window;
	int key;
	int scancode;
	int action;
	int mods;
};

std::ostream& operator<<(std::ostream& out, const KeyEvent& e);

struct MouseButtonEvent {
	GLFWwindow* window;
	int button;
	int action;
	int mods;
};

std::ostream& operator<<(std::ostream& out, const MouseButtonEvent& e);

struct MousePositionEvent {
	GLFWwindow* window;
	double xpos;
	double ypos;
};

std::ostream& operator<<(std::ostream& out, const MousePositionEvent& e);

struct ScrollEvent {
	GLFWwindow* window;
	double xoffset;
	double yoffset;
};

std::ostream& operator<<(std::ostream& out, const ScrollEvent& e);

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
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void mousePositionCallback(GLFWwindow* window, double xpos, double ypos);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	GLFWwindow* _glfwWindow = nullptr;
	bool _windowResized = false;

	bool _discardMouseEvents = false;
	bool _discardKeyboardEvents = false;

	EventRouter* _eventRouter;
public:
	static void pollEvents();
	static void initWindowingSystem();
	
	Window() = delete;
	Window(const Window&) = delete;
	Window(
		EventRouter& eventRouter,
		std::string title,
		uint32_t initialWidth,
		uint32_t initialHeight,
		const std::string& iconPath
	);
	~Window();

	bool shouldClose();
	bool wasResized() { return _windowResized; }
	void clearResized() { _windowResized = false; }
	void pauseWhileMinimized();
	WindowSize size();
	EventRouter& eventRouter() { return *_eventRouter; }
	void title(const std::string& newTitle);

	// In majority of cases should not use this. This function is used for
	// integration with ImGui.
	GLFWwindow* internalWindow() const { return _glfwWindow; }

	// Sets whether mouse/keyboard events will be discarded by the window's
	// event system.
	void discardMouseEvents(bool b) { _discardMouseEvents = b; }
	void discardKeyboardEvents(bool b) { _discardKeyboardEvents = b; }

	// Gets the window's current cursor mode.
	int cursorMode();

	// Increments the current cursor position. Used for a couple UI tricks.
	void incCursorPosition(double xinc, double yinc);

	vk::raii::SurfaceKHR createSurface(vk::raii::Instance& instance);
};
}
