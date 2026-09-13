#pragma once
#include "vk/camera.h"
#include "window.h"

#include <unordered_map>

namespace g3d {
class CameraController {
private:
	glm::vec3 initialCenter;
	glm::vec3 initialPosition;
	bool hasPreviousPos = false;
	double lastxpos = 0.0;
	double lastypos = 0.0;

	std::unordered_map<int, bool> directions {
		{GLFW_KEY_W, false},
		{GLFW_KEY_A, false},
		{GLFW_KEY_S, false},
		{GLFW_KEY_D, false},
		{GLFW_KEY_LEFT_SHIFT, false},
		{GLFW_KEY_SPACE, false}
	};

	void handleMouseButtonEvent(const MouseButtonEvent& e);
	void handleKeyEvent(const KeyEvent& e);
	void handleMousePositionEvent(const MousePositionEvent& e);
	void handleScrollEvent(const ScrollEvent& e);

	CompoundMethodEventHandler<CameraController,
		MouseButtonEvent,
		KeyEvent,
		MousePositionEvent,
		ScrollEvent
	> _eventHandlers { this,
		std::mem_fn(handleMouseButtonEvent),
		std::mem_fn(handleKeyEvent),
		std::mem_fn(handleMousePositionEvent),
		std::mem_fn(handleScrollEvent)
	};

	Window* _window;
	Camera _camera;

	void freeCamUpdate();
public:
	static inline const float defaultSensitivity = 0.005f;
	static inline const float defaultMoveSpeed = 0.1f;
	static inline const float defaultScrollSpeed = 0.5f;

	float sensitivity = defaultSensitivity;
	float moveSpeed = defaultMoveSpeed;
	float scrollSpeed = defaultScrollSpeed;

	CameraController(
		const glm::vec3& position,
		const glm::vec3& center,
		Window& window
	)
	: _window { &window },
	  _camera { CameraMode::fixedLook },
	  initialPosition { position },
	  initialCenter { center }
	{
		_camera.position = position;
		_camera.lookPosition = center;
		_camera.lookAt(center);

		_eventHandlers.addToRouter(window.eventRouter());
	}

	Camera& camera() { return _camera; }
	const Camera& camera() const { return _camera; }
	void update();

	bool mouseCaptured();
	void reset();
	CameraMode mode() const { return _camera.mode; }
	void mode(const CameraMode& mode);
	void align(const glm::vec3& axis);
};
}
