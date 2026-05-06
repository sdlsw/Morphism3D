#include "camera_control.h"
#include "container.h"
#include "graph.h"
#include "ui.h"
#include "vk_renderer.h"
#include "window.h"

#include <chrono>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>

static const uint32_t WINDOW_INITIAL_WIDTH = 800;
static const uint32_t WINDOW_INITIAL_HEIGHT = 600;
static const std::string APPLICATION_NAME = "graph3d";
static const std::string ENGINE_NAME = APPLICATION_NAME;

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

class ScrollDumper : public g3d::EventHandler<g3d::ScrollEvent> {
public:
	const std::string _name = "ScrollDumper";

	const std::string& name() const override { return _name; }

	void body(g3d::ScrollEvent& e) override {
		std::cout << "ScrollEvent: " << e.xoffset << ", " << e.yoffset << std::endl;
	}

	ScrollDumper(g3d::Window& window) {
		window.eventSystem().registerHandler(*this);
	}
};

g3d::RenderObject createAxes(g3d::GraphDevice& device, g3d::Renderer& renderer) {
	const float arrowLength = 1.2f;
	const float arrowWidth = 0.1f;
	const float arrowTipLength = 0.1f;

	glm::vec3 xcolor {1.0f, 0.0f, 0.0f};
	glm::vec3 ycolor {0.0f, 1.0f, 0.0f};
	glm::vec3 zcolor {0.2f, 0.2f, 1.0f};

	return {
		device,
		renderer,
		{
			device,
			{
				// X arrow
				{{0.0f, 0.0f, 0.0f}, xcolor},
				{{arrowLength, 0.0f, 0.0f}, xcolor},
				{{arrowLength-arrowTipLength, arrowWidth, 0.0f}, xcolor},
				{{arrowLength-arrowTipLength, -arrowWidth, 0.0f}, xcolor},

				// Y arrow
				{{0.0f, 0.0f, 0.0f}, ycolor},
				{{0.0f, arrowLength, 0.0f}, ycolor},
				{{arrowWidth, arrowLength-arrowTipLength, 0.0f}, ycolor},
				{{-arrowWidth, arrowLength-arrowTipLength, 0.0f}, ycolor},

				// Z arrow
				{{0.0f, 0.0f, 0.0f}, zcolor},
				{{0.0f, 0.0f, arrowLength}, zcolor},
				{{arrowWidth, 0.0f, arrowLength-arrowTipLength}, zcolor},
				{{-arrowWidth, 0.0f, arrowLength-arrowTipLength}, zcolor}
			},
			{
				0, 1, 1, 2, 1, 3,
				4, 5, 5, 6, 5, 7,
				8, 9, 9, 10, 9, 11
			}
		},
		{}
	};
}

class ConstantRotation {
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> _start;
	float _radiansPerSecond;
	glm::vec3 _axis;
public:
	ConstantRotation(float radiansPerSecond, const glm::vec3& axis) :
		_start { now() },
		_radiansPerSecond { radiansPerSecond },
		_axis { axis } {}

	glm::mat4 matrix() {
		return glm::rotate({1.0f}, secondsSince(_start) * _radiansPerSecond, _axis);
	}
};

// The function to graph. TODO let the user specify this
struct TestFunc {
	float eval(float x, float y) const {
		return 0.3f*x*y;
	}
};

void mainloop(g3d::GraphDevice& device, g3d::Renderer& renderer) {
	g3d::CameraController camController {
		{2.5f, -2.5f, 2.5f}, // cam position
		{0.0f, 0.0f, 0.0f},  // initial center
		device.window()
	};

	// Use last 50 frames to calculate average FPS
	g3d::MovingAverage<float> avgFps { 50 };

	g3d::RenderSettings renderSettings;

	float gridLoft = 0.002f;
	unsigned int cells = 20;
	float range = 3.0f;

	g3d::Graph<TestFunc> graph { {}, cells, range };
	auto surfaceObject = graph.makeSurfaceObject(device, renderer);
	auto gridObject = graph.makeGridObject(device, renderer);
	auto normalsObject = graph.makeNormalObject(device, renderer);

	auto axesObject = createAxes(device, renderer);

	g3d::Ui ui { camController, renderSettings };

	while (!device.window().shouldClose()) {
		auto frameStartTime = now();

		g3d::Window::pollEvents();
		g3d::imGuiNewFrame();
		g3d::imGuiHandleControlExclusivity(device.window(), camController);

		// UI updates.
		ui.show();

		// Entity and camera updates.
		camController.update();

		// Draw the frame.
		auto& renderContext = renderer.beginFrame(camController.camera());
		if (!renderer.inFrame()) {
			// Indicates we failed to begin the frame for some
			// outside reason - probably window resize
			continue;
		}
		renderer.setMode(g3d::RenderMode::line);

		if (renderSettings.renderAxes) {
			axesObject.record(renderContext);
		}

		// TODO This is really dumb but it doesn't look too terrible...
		// Look into using textures for the grid, maybe.
		if (renderSettings.renderGrid) {
			gridObject.transform.translation = {0.0f, 0.0f, gridLoft};
			gridObject.record(renderContext);
			gridObject.transform.translation = {0.0f, 0.0f, -gridLoft};
			gridObject.record(renderContext);
		}

		if (renderSettings.renderNormals) {
			normalsObject.record(renderContext);
		}

		renderer.setMode(g3d::RenderMode::triangle);
		surfaceObject.record(renderContext);

		g3d::imGuiRecord(renderContext);
		renderer.endFrame();

		// TODO I don't think this is *quite* accurate since endFrame()
		// is only the submit point for rendering commands; the frame
		// isn't actually finished drawing until a bit later. For now
		// seems fine since there's almost 0 work to do and we're
		// hitting vsync limit.
		avgFps.put(1.0f / secondsSince(frameStartTime));

		device.window().title(std::format(
			"{} | {:.2f} FPS",
			APPLICATION_NAME,
			avgFps.get()
		));
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
		ScrollDumper scrollDumper { window };
		scrollDumper.disable();

		g3d::GraphDevice graphDevice { vkTop, window };
		g3d::Renderer renderer { graphDevice };
		g3d::ImGuiWrapper imGui { graphDevice, renderer };

		mainloop(graphDevice, renderer);
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
