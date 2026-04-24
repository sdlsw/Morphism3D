#pragma once
#include "vk.h"
#include "window.h"

#include <unordered_map>

namespace g3d {
class CameraController {
private:
	struct _InternalState {
		glm::vec3 initialCenter;
		glm::vec3 initialPosition;
		bool hasPreviousPos = false;
		double lastxpos = 0.0;
		double lastypos = 0.0;
		float sensitivity = 0.005f;
		float moveSpeed = 0.5f;
		float scrollSpeed = 0.5f;

		std::unordered_map<int, bool> directions {
			{GLFW_KEY_W, false},
			{GLFW_KEY_A, false},
			{GLFW_KEY_S, false},
			{GLFW_KEY_D, false},
			{GLFW_KEY_LEFT_SHIFT, false},
			{GLFW_KEY_SPACE, false}
		};
	};

	class _MouseHandler : public EventHandler<MouseButtonEvent> {
	private:
		const std::string _name = "CameraController::_MouseHandler";

	public:
		const std::string& name() const override { return _name; }
		void body(MouseButtonEvent& e) override;
	};

	class _KeyHandler : public EventHandler<KeyEvent> {
	private:
		const std::string _name = "CameraController::_KeyHandler";
		_InternalState* _state;
		Camera* _camera;

	public:
		const std::string& name() const override { return _name; }
		void body(KeyEvent& e) override;
		_KeyHandler(_InternalState* state, Camera* camera) : _state {state}, _camera {camera} {}
	};

	class _PosHandler : public EventHandler<MousePositionEvent> {
	private:
		const std::string _name = "CameraController::_PosHandler";
		_InternalState* _state;
		Camera* _camera;
		void handleFixedLook(float xoffset, float yoffset);
		void handleForward(float xoffset, float yoffset);

	public:
		const std::string& name() const override { return _name; }
		void body(MousePositionEvent& e) override;
		_PosHandler(_InternalState* state, Camera* camera) : _state {state}, _camera {camera} {}
	};

	class _ScrollHandler : public EventHandler<ScrollEvent> {
	private:
		const std::string _name = "CameraController::_ScrollHandler";
		_InternalState* _state;
		Camera* _camera;

	public:
		const std::string& name() const override { return _name; }
		void body(ScrollEvent& e) override;
		_ScrollHandler(_InternalState* state, Camera* camera) : _state {state}, _camera {camera} {}
	};

	Camera _camera;
	_InternalState _state;
	_MouseHandler mouseHandler;
	_KeyHandler keyHandler { &_state, &_camera };
	_PosHandler posHandler { &_state, &_camera };
	_ScrollHandler scrollHandler { &_state, &_camera };

public:
	CameraController(
		const glm::vec3& position,
		const glm::vec3& center,
		Window& window
	) : _camera { CameraMode::fixedLook, position, center } {
		_state.initialPosition = position;
		_state.initialCenter = center;
		window.eventSystem().registerHandler(mouseHandler);
		window.eventSystem().registerHandler(keyHandler);
		window.eventSystem().registerHandler(posHandler);
		window.eventSystem().registerHandler(scrollHandler);
	}

	Camera& camera() { return _camera; }
	const Camera& camera() const { return _camera; }
	void update();
};
}
