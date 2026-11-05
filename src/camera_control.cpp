#include "camera_control.h"

bool windowMouseCaptured(GLFWwindow* window) {
	return glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
}

bool windowMouseCaptured(g3d::Window& window) {
	return window.cursorMode() == GLFW_CURSOR_DISABLED;
}

void windowCaptureMouse(GLFWwindow* window, bool captured) {
	int mode = captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
	glfwSetInputMode(window, GLFW_CURSOR, mode);
}

namespace g3d {
void CameraController::handleMouseButtonEvent(const MouseButtonEvent& e) {
	if (e.action == GLFW_RELEASE) return;

	bool mouseCaptured = windowMouseCaptured(e.window);

	if (glfwGetWindowAttrib(e.window, GLFW_HOVERED) && !mouseCaptured) {
		windowCaptureMouse(e.window, true);
	} else if (mouseCaptured) {
		windowCaptureMouse(e.window, false);
		hasPreviousPos = false;
	}
}

void CameraController::handleKeyEvent(const KeyEvent& e) {
	if (!windowMouseCaptured(e.window)) return;

	// Escape cursor capture
	if (e.key == GLFW_KEY_ESCAPE && e.action == GLFW_PRESS) {
		windowCaptureMouse(e.window, false);
		hasPreviousPos = false;
		return;
	}

	// Camera reset
	if (e.key == GLFW_KEY_R && e.action == GLFW_PRESS) {
		reset();
		return;
	}

	// Switch camera mode
	if (e.key == GLFW_KEY_E && e.action == GLFW_PRESS) {
		if (mode() == CameraMode::fixedLook) {
			mode(CameraMode::forward);
		} else {
			mode(CameraMode::fixedLook);
		}
	}

	if (directions.contains(e.key)) {
		if (e.action == GLFW_PRESS) {
			directions[e.key] = true;
		}

		if (e.action == GLFW_RELEASE) {
			directions[e.key] = false;
		}
	}
}

void CameraController::handleMousePositionEvent(const MousePositionEvent& e) {
	if (windowMouseCaptured(e.window)) {
		if (hasPreviousPos) {
			float xoffset = static_cast<float>(e.xpos - lastxpos);
			float yoffset = static_cast<float>(e.ypos - lastypos);

			// When upside down, need to mirror horizontal angle
			// increments to keep the apparent rotation direction
			// consistent. TODO allow this behavior to be toggled?
			float upsideDownCorrection = _camera.up().z < 0.0f ? -1.0f : 1.0f;

			_camera.angles.x += -sensitivity * upsideDownCorrection * xoffset;
			_camera.angles.y += -sensitivity * yoffset;
		}

		lastxpos = e.xpos;
		lastypos = e.ypos;
		hasPreviousPos = true;
	}
}

void CameraController::handleScrollEvent(const ScrollEvent& e) {
	// Scroll control only enabled in fixedLook mode
	if (mode() == CameraMode::forward) return;

	glm::vec3 move = _camera.forward() * scrollSpeed * static_cast<float>(e.yoffset);

	if (e.yoffset > 0) {
		// Make sure we don't overshoot center if moving towards it
		float dToNew = glm::length(move);
		float dToCenter = glm::distance(_camera.position, _camera.lookPosition);

		if (dToCenter < dToNew) return;
	}

	_camera.position += move;
}

void CameraController::freeCamUpdate() {
	bool w = directions[GLFW_KEY_W];
	bool a = directions[GLFW_KEY_A];
	bool s = directions[GLFW_KEY_S];
	bool d = directions[GLFW_KEY_D];
	bool up = directions[GLFW_KEY_SPACE];
	bool down = directions[GLFW_KEY_LEFT_SHIFT];

	glm::vec3 move { 0.0f, 0.0f, 0.0f};

	if (w ^ s) {
		auto direction = _camera.forward();
		float polarity = w ? 1.0f : -1.0f;
		move += direction * polarity;
	}

	if (a ^ d) {
		auto direction = _camera.right();
		float polarity = d ? 1.0f : -1.0f;
		move += direction * polarity;
	}

	if (up ^ down) {
		auto direction = _camera.up();
		float polarity = up ? 1.0f : -1.0f;
		move += direction * polarity;
	}

	_camera.position += move * moveSpeed;
}

void CameraController::update() {
	// Ignore first person movement if camera is looking at a fixed point
	if (_camera.mode == CameraMode::forward) {
		freeCamUpdate();
	}

	_camera.update();
}

bool CameraController::mouseCaptured() {
	return windowMouseCaptured(*_window);
}

void CameraController::reset() {
	_camera.position = initialPosition;
	_camera.lookAt(initialCenter);
}

void CameraController::mode(const CameraMode& mode) {
	_camera.mode = mode;
}

void CameraController::align(const glm::vec3& axis) {
	float dist = glm::distance(initialCenter, _camera.position);
	_camera.position = initialCenter + axis*dist;
	_camera.lookAt(initialCenter);
}
}
