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
		CameraController* _this;

	public:
		const std::string& name() const override { return _name; }
		void body(KeyEvent& e) override;
		_KeyHandler(CameraController* parent) : _this { parent } {}
	};

	class _PosHandler : public EventHandler<MousePositionEvent> {
	private:
		const std::string _name = "CameraController::_PosHandler";
		CameraController* _this;

	public:
		const std::string& name() const override { return _name; }
		void body(MousePositionEvent& e) override;
		_PosHandler(CameraController* parent) : _this { parent } {}
	};

	class _ScrollHandler : public EventHandler<ScrollEvent> {
	private:
		const std::string _name = "CameraController::_ScrollHandler";
		CameraController* _this;

	public:
		const std::string& name() const override { return _name; }
		void body(ScrollEvent& e) override;
		_ScrollHandler(CameraController* parent) : _this { parent} {}
	};

	Window* _window;
	Camera _camera;
	_MouseHandler mouseHandler;
	_KeyHandler keyHandler { this };
	_PosHandler posHandler { this };
	_ScrollHandler scrollHandler { this };

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

		window.eventSystem().registerHandler(mouseHandler);
		window.eventSystem().registerHandler(keyHandler);
		window.eventSystem().registerHandler(posHandler);
		window.eventSystem().registerHandler(scrollHandler);
	}

	Camera& camera() { return _camera; }
	const Camera& camera() const { return _camera; }
	void update();
	void disable();
	void enable();

	bool mouseCaptured();
	void reset();
	CameraMode mode() const { return _camera.mode; }
	void mode(const CameraMode& mode);
	void align(const glm::vec3& axis);
};
}
