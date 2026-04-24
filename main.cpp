#include "camera_control.h"
#include "container.h"
#include "vk.h"
#include "window.h"

#include <chrono>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

static const uint32_t WINDOW_INITIAL_WIDTH = 800;
static const uint32_t WINDOW_INITIAL_HEIGHT = 600;
static const std::string APPLICATION_NAME = "graph3d";
static const std::string ENGINE_NAME = APPLICATION_NAME;

static const glm::vec3 INITIAL_LOOK_POSITION {0.0f, 0.0f, 0.0f};

g3d::VkTop init_top() {
	vk::ApplicationInfo appInfo {
		.pApplicationName = APPLICATION_NAME.c_str(),
		.applicationVersion = 1,
		.pEngineName = ENGINE_NAME.c_str(),
		.engineVersion = 1,
		.apiVersion = VK_API_VERSION_1_1
	};
	g3d::VkTop vk_top { appInfo };
	return vk_top;
}

std::chrono::time_point<std::chrono::high_resolution_clock> now() {
	return std::chrono::high_resolution_clock::now();
}

float secondsSince(const std::chrono::time_point<std::chrono::high_resolution_clock>& before) {
	auto after = now();
	return std::chrono::duration<float, std::chrono::seconds::period>(after - before).count();
}

class TestObject {
private:
	// Radians per second
	float _rotationSpeed;
	g3d::RenderObject _obj;
	glm::vec3 _translation;

public:
	TestObject(
		g3d::GraphDevice& device,
		g3d::Renderer& renderer,
		float rotationSpeed,
		const glm::vec3& translation
	) :
		_rotationSpeed {rotationSpeed},
		_obj {
			device,
			renderer,
			{
				device,
				{
					{{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
					{{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
					{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
					{{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}
				},
				{0, 1, 1, 2, 2, 3, 3, 0}
			},
			{ translation }
		} {}

	void updateAndRender(g3d::RenderContext& ctx) {
		static auto startTime = now();

		_obj.transform.rotation = glm::rotate(
			{1.0f},
			secondsSince(startTime) * _rotationSpeed,
			glm::vec3(0.0f, 0.0f, 1.0f)
		);

		_obj.update(ctx);
		_obj.record(ctx);
	}
};

class CursorPosDumper : public g3d::EventHandler<g3d::MousePositionEvent> {
public:
	const std::string _name = "CursorPosDumper";

	const std::string& name() const override { return _name; }

	void body(g3d::MousePositionEvent& e) override {
		std::cout << "MousePositionEvent: " << e.xpos << ", " << e.ypos << std::endl;
	}

	CursorPosDumper(g3d::Window& window) {
		window.eventSystem().registerHandler(*this);
	}
};

class KeyDumper : public g3d::EventHandler<g3d::KeyEvent> {
public:
	const std::string _name = "KeyDumper";

	const std::string& name() const override { return _name; }

	void body(g3d::KeyEvent& e) override {
		std::cout << "KeyEvent: " << e.key << ", " << e.action << std::endl;
	}

	KeyDumper(g3d::Window& window) {
		window.eventSystem().registerHandler(*this);
	}
};

class CameraResetHandler : public g3d::EventHandler<g3d::KeyEvent> {
private:
	g3d::Camera* _camera;

public:
	const std::string _name = "CameraResetHandler";
	const std::string& name() const override { return _name; }

	void body(g3d::KeyEvent& e) override {
		if (e.key == GLFW_KEY_R && e.action == GLFW_PRESS) {
			_camera->lookAt(INITIAL_LOOK_POSITION);
		}
	}

	CameraResetHandler(g3d::Window& window, g3d::Camera& camera) : _camera {&camera} {
		window.eventSystem().registerHandler(*this);
	}
};

void mainloop(g3d::GraphDevice& device, g3d::Renderer& renderer) {
	g3d::CameraController camController {
		g3d::CameraMode::fixedLook,
		{4.0f, 4.0f, 4.0f}, // cam position
		INITIAL_LOOK_POSITION,
		device.window()
	};
	CameraResetHandler camResetter { device.window(), camController.camera() };
	g3d::MovingAverage<float> avgFps { 50 };

	TestObject obj1 { device, renderer, glm::radians(90.0f), {0.0f, 0.0f, 0.0f} };
	TestObject obj2 { device, renderer, glm::radians(-120.0f), {0.0f, 0.0f, 1.0f} };
	TestObject obj3 { device, renderer, glm::radians(120.0f), {2.0f, 0.0f, 0.0f} };

	while (!device.window().shouldClose()) {
		auto frameStartTime = now();

		g3d::Window::pollEvents();
		camController.update();

		auto& renderContext = renderer.beginFrame(camController.camera());
		obj1.updateAndRender(renderContext);
		obj2.updateAndRender(renderContext);
		obj3.updateAndRender(renderContext);
		renderer.endFrame();

		// TODO I don't think this is *quite* accurate since endFrame()
		// is only the submit point for rendering commands; the frame
		// isn't actually finished drawing until a bit later. For now
		// seems fine since there's almost 0 work to do and we're
		// hitting vsync limit.
		avgFps.put(1.0f / secondsSince(frameStartTime));

		device.window().title(std::format("{} | {:.2f} FPS", APPLICATION_NAME, avgFps.get()));
	}

	device.logicalDevice().waitIdle();
}

int main() {
	try {
		g3d::Window::initWindowingSystem();

		auto vkTop = init_top();
		g3d::Window window {
			vkTop.appInfo().pApplicationName,
			WINDOW_INITIAL_WIDTH,
			WINDOW_INITIAL_HEIGHT
		};

		CursorPosDumper posDumper { window };
		posDumper.disable();

		KeyDumper keyDumper { window };
		keyDumper.disable();

		g3d::GraphDevice graphDevice { vkTop, window };
		g3d::Renderer renderer { graphDevice };

		mainloop(graphDevice, renderer);
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
