#include "camera_control.h"

namespace g3d {
void CameraController::_MouseHandler::body(MouseButtonEvent& e) {
	if (glfwGetWindowAttrib(e.window, GLFW_HOVERED)) {
		glfwSetInputMode(e.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
}

void CameraController::_KeyHandler::body(KeyEvent& e) {
	int inMode = glfwGetInputMode(e.window, GLFW_CURSOR);
	if (inMode != GLFW_CURSOR_DISABLED) return;

	if (e.key == GLFW_KEY_ESCAPE && e.action == GLFW_PRESS) {
		glfwSetInputMode(e.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		_state->hasPreviousPos = false;
		return;
	}

	if (_state->directions.contains(e.key)) {
		if (e.action == GLFW_PRESS) {
			_state->directions[e.key] = true;
		}

		if (e.action == GLFW_RELEASE) {
			_state->directions[e.key] = false;
		}
	}
}

void CameraController::_PosHandler::body(MousePositionEvent& e) {
	int inMode = glfwGetInputMode(e.window, GLFW_CURSOR);
	if (inMode == GLFW_CURSOR_DISABLED) {
		if (_state->hasPreviousPos) {
			float xoffset = static_cast<float>(e.xpos - _state->lastxpos);
			float yoffset = static_cast<float>(e.ypos - _state->lastypos);

			_camera->rotateForward(
				-_state->sensitivity * xoffset,
				-_state->sensitivity * yoffset
			);
		}

		_state->lastxpos = e.xpos;
		_state->lastypos = e.ypos;
		_state->hasPreviousPos = true;
	}
}

void CameraController::update() {
	bool w = _state.directions[GLFW_KEY_W];
	bool a = _state.directions[GLFW_KEY_A];
	bool s = _state.directions[GLFW_KEY_S];
	bool d = _state.directions[GLFW_KEY_D];

	if (w ^ s) {
		auto direction = _camera.forward();
		float polarity = w ? 1.0f : -1.0f;

		_camera.position(_camera.position() + direction * polarity * _state.speed);
	}

	if (a ^ d) {
		auto direction = _camera.right();
		float polarity = d ? 1.0f : -1.0f;

		_camera.position(_camera.position() + direction * polarity * _state.speed);
	}
}
}
