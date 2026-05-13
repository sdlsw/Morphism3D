#include "camera_control.h"
#include "container.h"
#include "graph.h"
#include "primitive.h"
#include "ui.h"
#include "vk/renderer.h"
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

class Axes {
private:
	static constexpr float arrowLength = 1.2f;
	static constexpr float arrowWidth = 0.1f;
	static constexpr float arrowTipLength = 0.1f;
	static constexpr unsigned int X = 0;
	static constexpr unsigned int Y = 1;
	static constexpr unsigned int Z = 2;
	static constexpr unsigned int N_AXES = 3;

	g3d::StaticMesh _arrow;
	std::vector<g3d::StaticVertexAttributes<g3d::Color>> _colors;
	std::array<g3d::Entity, N_AXES> _axisEntities;

	g3d::StaticMesh createArrowMesh(g3d::Renderer& renderer) {
		// This arrow is pointing in the X direction.
		std::vector<g3d::Position> positions {
			{0.0f, 0.0f, 0.0f},
			{arrowLength, 0.0f, 0.0f},
			{arrowLength-arrowTipLength, arrowWidth, 0.0f},
			{arrowLength-arrowTipLength, -arrowWidth, 0.0f},
			{arrowLength-arrowTipLength, 0.0f, arrowWidth},
			{arrowLength-arrowTipLength, 0.0f, -arrowWidth}
		};
		std::vector<uint16_t> indices { 0, 1, 1, 2, 1, 3, 1, 4, 1, 5 };

		return {
			renderer,
			positions,
			indices
		};
	}

	std::vector<g3d::StaticVertexAttributes<g3d::Color>> createColors(g3d::Renderer& renderer) {
		std::vector<g3d::StaticVertexAttributes<g3d::Color>> out;

		for (unsigned int i = 0; i < N_AXES; i++) {
			g3d::Color c { 0.0f, 0.0f, 0.0f};
			c.vec[i] = 1.0f;
			c.vec[(i + 1) % N_AXES] = 0.1f;
			c.vec[(i + 2) % N_AXES] = 0.1f;
			std::vector<g3d::Color> solidColor { _arrow.positionCount(), c };
			out.emplace_back(renderer, solidColor);
		}

		return out;
	}

	g3d::Transform axisTransform(unsigned int n) {
		glm::mat4 rotation {1.0f};

		if (n == Y) {
			glm::vec3 rotAxis {0.0f, 0.0f, 1.0f};
			rotation = glm::rotate({1.0f}, glm::radians(90.0f), rotAxis);
		} else if (n == Z) {
			glm::vec3 rotAxis {0.0f, 1.0f, 0.0f};
			rotation = glm::rotate({1.0f}, glm::radians(-90.0f), rotAxis);
		}

		return {{}, g3d::Transform::default_scale, rotation};
	}

	void populateAxisEntity(g3d::Renderer& renderer, g3d::Entity& entity, unsigned int n) {
		g3d::populateStaticEntity(renderer, entity, axisTransform(n), _arrow, _colors[n]);
	}
public:
	Axes(g3d::Renderer& renderer)
	: _arrow { createArrowMesh(renderer) },
	  _colors { createColors(renderer) }
	{
		for (unsigned int i = 0; i < N_AXES; i++) {
			populateAxisEntity(renderer, _axisEntities[i], i);
		}
	}

	void render() {
		for (auto& axis : _axisEntities) {
			axis.render();
		}
	}
};

class LightObject {
private:
	g3d::StaticMesh _mesh;
	g3d::StaticVertexAttributes<g3d::Color> _colors;
	g3d::Entity _entity;

public:
	LightObject(g3d::Renderer& renderer)
	: _mesh { cubeMesh(renderer, 0.1f) },
	  _colors { solidColor(_mesh, {1.0f, 1.0f, 1.0f}) }
	{
		g3d::populateStaticEntity(renderer, _entity, {}, _mesh, _colors);
	}

	void update(const g3d::Light& light) {
		_entity.requireComponent<g3d::TransformComponent>().transform.translation = light.position;
	}

	void render() {
		_entity.render();
	}
};

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

void mainloop(g3d::Renderer& renderer) {
	auto& device = renderer.graphDevice();

	g3d::CameraController camController {
		{2.5f, -2.5f, 2.5f}, // cam position
		{0.0f, 0.0f, 0.0f},  // initial center
		device.window()
	};

	// Use last 50 frames to calculate average FPS
	g3d::MovingAverage<float> avgFps { 50 };

	g3d::RenderSettings renderSettings;

	unsigned int cells = 20;
	float range = 3.0f;

	g3d::Graph<TestFunc> graph { {}, cells, range };
	g3d::GraphEntities<TestFunc> graphEntities { renderer, graph };

	Axes axes { renderer };

	g3d::WithInitial<g3d::Light> light {{
		{0.0f, 0.0f, 1.5f}, // position
		{1.0f, 1.0f, 1.0f}, // color
		1.0f
	}};
	LightObject lightObject { renderer };

	g3d::Ui ui {
		camController,
		renderSettings,
		light,
		graphEntities.surfaceMaterial()
	};

	while (!device.window().shouldClose()) {
		auto frameStartTime = now();

		g3d::Window::pollEvents();
		g3d::imGuiNewFrame();
		g3d::imGuiHandleControlExclusivity(device.window(), camController);

		// UI updates.
		ui.show();

		// Entity and camera updates.
		camController.update();
		lightObject.update(light);

		// Draw the frame.
		renderer.beginFrame(camController.camera(), light);
		if (!renderer.inFrame()) {
			// Indicates we failed to begin the frame for some
			// outside reason - probably window resize
			continue;
		}
		renderer.setMode(g3d::RenderMode::line);

		if (renderSettings.renderAxes) {
			axes.render();
		}

		// TODO This is really dumb but it doesn't look too terrible...
		// Look into using textures for the grid, maybe.
		bool shouldRenderSurfaceGrid = (
			renderSettings.renderGrid &&
			renderSettings.graphRenderMode == g3d::GraphRenderMode::surface
		);
		if (shouldRenderSurfaceGrid) {
			graphEntities.gridTop().render();
			graphEntities.gridBottom().render();
		}

		if (renderSettings.renderNormals) {
			graphEntities.normals().render();
		}

		if (renderSettings.graphRenderMode == g3d::GraphRenderMode::wireframe) {
			graphEntities.wireframe().render();
		}

		renderer.setMode(g3d::RenderMode::triangle);
		if (renderSettings.renderLightObject) {
			lightObject.render();
		}

		renderer.setMode(g3d::RenderMode::litTriangle);
		if (renderSettings.graphRenderMode == g3d::GraphRenderMode::surface) {
			graphEntities.surface().render();
		}

		g3d::imGuiRecord(renderer);
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

		mainloop(renderer);
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
