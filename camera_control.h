#pragma once
#include "vk.h"
#include "window.h"

#include <unordered_map>

namespace g3d {
class CameraController {
private:
	struct _InternalState {
		bool hasPreviousPos = false;
		double lastxpos = 0.0;
		double lastypos = 0.0;
		float sensitivity = 0.005f;
		float speed = 0.5f;

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
	public:
		const std::string _name = "CameraController::_MouseHandler";
		const std::string& name() const override { return _name; }
		void body(MouseButtonEvent& e) override;
	};

	class _KeyHandler : public EventHandler<KeyEvent> {
	public:
		_InternalState* _state;
		Camera* _camera;
		const std::string _name = "CameraController::_KeyHandler";
		const std::string& name() const override { return _name; }
		void body(KeyEvent& e) override;
		_KeyHandler(_InternalState* state, Camera* camera) : _state {state}, _camera {camera} {}
	};

	class _PosHandler : public EventHandler<MousePositionEvent> {
	public:
		_InternalState* _state;
		Camera* _camera;
		const std::string _name = "CameraController::_PosHandler";
		const std::string& name() const override { return _name; }
		void body(MousePositionEvent& e) override;
		_PosHandler(_InternalState* state, Camera* camera) : _state {state}, _camera {camera} {}
	};

	Camera _camera;
	_InternalState _state;
	_MouseHandler mouseHandler;
	_KeyHandler keyHandler { &_state, &_camera };
	_PosHandler posHandler { &_state, &_camera };

public:
	CameraController(
		CameraMode camMode,
		const glm::vec3& position,
		const glm::vec3& look,
		Window& window
	) : _camera { camMode, position, look } {
		_camera.mode(CameraMode::forward);
		window.inputSystem().registerHandler(mouseHandler);
		window.inputSystem().registerHandler(keyHandler);
		window.inputSystem().registerHandler(posHandler);
	}

	Camera& camera() { return _camera; }
	void update();
};
}
