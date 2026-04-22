#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <iostream>
#include <unordered_map>
#include <string>

const uint64_t INPUT_HANDLER_NO_ID = 0;
const std::string INPUT_UNNAMED_HANDLER = "unnamed";

namespace g3d {
struct KeyEvent {
	GLFWwindow* window;
	int key;
	int scancode;
	int action;
	int mods;
};

struct MouseButtonEvent {
	GLFWwindow* window;
	int button;
	int action;
	int mods;
};

struct MousePositionEvent {
	GLFWwindow* window;
	double xpos;
	double ypos;
};

class InputSystem;
template<typename E> class EventHandler;

class BaseEventHandler {
	friend class InputSystem;

	template<typename E>
	friend class EventHandler;

private:
	bool _enabled = true;
	uint64_t _id = INPUT_HANDLER_NO_ID;
	InputSystem* _inputSystem = nullptr;

	void link(InputSystem* isys, uint64_t id) {
		_id = id;
		_inputSystem = isys;
	}

	void unlink() {
		_id = INPUT_HANDLER_NO_ID;
		_inputSystem = nullptr;
	}

	bool isLinked() { return _inputSystem != nullptr; }

protected:
	InputSystem& inputSystem() {
		if (isLinked()) {
			return *_inputSystem;
		}

		throw std::runtime_error("Cannot call inputSystem() if handler not linked.");
	}

public:
	bool isEnabled() { return _enabled; }
	void enable() { _enabled = true; }
	void disable() { _enabled = false; }
};

template<typename E>
class EventHandler : public BaseEventHandler {
public:
	~EventHandler() {
		// Automatically deregister handlers when they go out of scope.
		// This desctructor is defined here instead of BaseEventHandler
		// so that the compiler picks the correct instantiation of
		// InputSystem::deregisterHandler<E>().
		if (isLinked()) {
			inputSystem().deregisterHandler(*this);
		}
	}

	void handleEvent(E& e) {
		if (isEnabled()) body(e);
	}

	virtual void body(E& e) {}
	virtual const std::string& name() const { return INPUT_UNNAMED_HANDLER; }
};

class InputSystem {
private:
	uint64_t _nextId = INPUT_HANDLER_NO_ID + 1;

	// Collection of all registered handlers. Used to handle proper
	// destruction behavior.
	std::unordered_map<uint64_t, BaseEventHandler*> _allHandlers;

	// Categorized handlers. Used for actually handling events so we don't
	// need to iterate over all handler types for one type of event.
	std::unordered_map<uint64_t, EventHandler<KeyEvent>*> _keyHandlers;
	std::unordered_map<uint64_t, EventHandler<MouseButtonEvent>*> _mouseButtonHandlers;
	std::unordered_map<uint64_t, EventHandler<MousePositionEvent>*> _mousePositionHandlers;

	template<typename E>
	std::unordered_map<uint64_t, EventHandler<E>*>& getCategory() {
		throw std::runtime_error("Unknown event type.");
	}

	// Kind of gross, but having these specializations means we can avoid
	// duplicating registerHandler, deregisterHandler, handleEvent, and
	// EventHandler.
	template<>
	std::unordered_map<uint64_t, EventHandler<KeyEvent>*>& getCategory() {
		return _keyHandlers;
	}

	template<>
	std::unordered_map<uint64_t, EventHandler<MouseButtonEvent>*>& getCategory() {
		return _mouseButtonHandlers;
	}

	template<>
	std::unordered_map<uint64_t, EventHandler<MousePositionEvent>*>& getCategory() {
		return _mousePositionHandlers;
	}

public:
	InputSystem() = default;
	InputSystem(const InputSystem&) = delete;
	~InputSystem();

	template<typename E>
	uint64_t registerHandler(EventHandler<E>& handler) {
		if (handler.isLinked()) {
			throw std::runtime_error("Cannot register handler; already linked.");
		}

		uint64_t id = _nextId;
		_nextId++;

		auto& category = getCategory<E>();

		_allHandlers[id] = &handler;
		category[id] = &handler;
		handler.link(this, id);

		std::cerr << "InputSystem: Registered handler \"" << handler.name() << "\", id ";
		std::cerr << id << std::endl;

		return id;
	}

	template<typename E>
	void deregisterHandler(EventHandler<E>& handler) {
		if (!handler.isLinked()) {
			throw std::runtime_error("Cannot deregister handler that is not registered.");
		}

		if (handler._inputSystem != this) {
			throw std::runtime_error(
				"Cannot deregister handler belonging to another InputSystem."
			);
		}

		auto& category = getCategory<E>();
		uint64_t id = handler._id;

		_allHandlers.erase(handler._id);
		category.erase(handler._id);
		handler.unlink();

		std::cerr << "InputSystem: Deregistered handler \"" << handler.name() << "\", id ";
		std::cerr << id << std::endl;
	}

	template<typename E>
	void handleEvent(E& e) {
		auto& category = getCategory<E>();
		for (const auto& [id, handler] : category) {
			handler->handleEvent(e);
		}
	}
};

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

	GLFWwindow* _glfwWindow = nullptr;
	bool _windowResized = false;

	InputSystem _inputSystem;
public:
	static void pollEvents();
	static void initWindowingSystem();
	
	Window() = delete;
	Window(const Window&) = delete;
	Window(std::string title, uint32_t initialWidth, uint32_t initialHeight);
	~Window();

	bool shouldClose();
	WindowSize size();
	InputSystem& inputSystem() { return _inputSystem; }

	vk::raii::SurfaceKHR createSurface(vk::raii::Instance& instance);
};
}
