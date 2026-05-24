#include "camera_control.h"
#include "container.h"
#include "expression.h"
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
static const std::string APPLICATION_ICON = "icon.png";

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

class WobbleFunc {
private:
	float ofs = 0.0f;
	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

public:
	WobbleFunc() : startTime { now() } {}

	// Couple test funcs to swap between
	float rippler(float x, float y) const {
		return 0.2f*glm::sin(3*glm::sqrt(x*x + y*y) - 2*ofs);
	}

	float diagonalWave(float x, float y) const {
		return glm::sin(x+ofs)*glm::sin(y+ofs);
	}

	float eval(float x, float y) const {
		return diagonalWave(x, y);
	}

	void update() {
		ofs = secondsSince(startTime);
	}
};

class ExpressionFunc {
private:
	g3d::TokenRegistry _tokenRegistry;
	g3d::VariableStore _vars;
	std::chrono::time_point<std::chrono::high_resolution_clock> _startTime;
	std::unique_ptr<g3d::ParseNode> _parsedExpression;

public:
	ExpressionFunc()
	: _tokenRegistry { g3d::makeTokenRegistry() },
	  _startTime { now() }
	{}

	float eval(float x, float y) {
		if (!_parsedExpression) {
			return 0.0f;
		}

		_vars.set('x', x);
		_vars.set('y', y);
		return _parsedExpression.get()->eval();
	}

	void update() {
		_vars.set('t', secondsSince(_startTime));
	}

	void updateExpression(const std::string& expression) {
		g3d::Parser p { _tokenRegistry, _vars, expression };

		try {
			_parsedExpression.reset(new g3d::ParseNode(p.parse()));
		} catch (const std::exception& e) {
			std::cerr << "Failed to parse expression: " << expression << std::endl;
			std::cerr << e.what() << std::endl;
		}
	}
};

void buildArrow(g3d::MeshBuilder& b) {
	constexpr unsigned int hRes = 10;
	constexpr unsigned int cylVRes = 30;
	constexpr unsigned int coneVRes = hRes;
	constexpr float arrowLength = 1.2f;
	constexpr float arrowTipLength = 0.1f;
	constexpr float arrowRadius = 0.01f;
	constexpr float arrowTipRadius = arrowRadius * 2.0f;

	constexpr float cylLength = arrowLength - arrowTipLength;

	b.beginGroup();
	b.primitive<g3d::Cone>(
		{ arrowTipRadius, arrowTipLength, hRes, coneVRes },
		{ 0.0f, 0.0f, cylLength }
	);
	b.primitive<g3d::Cylinder>(
		{ arrowRadius, cylLength, hRes, cylVRes },
		{ 0.0f, 0.0f, cylLength / 2.0f }
	);
	b.endGroup();
}

g3d::SimpleLitObject buildAxes(g3d::Renderer& renderer) {
	constexpr g3d::Color xcolor { 0.6f, 0.1f, 0.1f };
	constexpr g3d::Color ycolor { 0.1f, 0.6f, 0.1f };
	constexpr g3d::Color zcolor { 0.1f, 0.1f, 0.6f };
	constexpr g3d::Color centerColor { 0.1f, 0.1f, 0.1f };

	auto builder = g3d::SimpleLitObject::mkBuilder(
		renderer,
		g3d::MeshBuilderMode::genColors
	);

	builder.setMaterial({ 0.4f, 0.2f, 0.7f, 8.0f });

	builder.setColor(xcolor);
	buildArrow(builder);
	builder.rot90Y(g3d::RotateDirection::CCW);

	builder.setColor(ycolor);
	buildArrow(builder);
	builder.rot90X(g3d::RotateDirection::CW);

	builder.setColor(zcolor);
	buildArrow(builder);

	builder.setColor(centerColor);
	builder.primitive<g3d::Sphere>({0.02f, 20, 20});

	return { builder };
}

g3d::SimpleUnlitObject buildLightObject(g3d::Renderer& renderer) {
	return {
		g3d::SimpleUnlitObject::mkBuilder(renderer)
			.setColor({1.0f, 1.0f, 1.0f})
			.primitive<g3d::Cube>({0.1f})
	};
}

g3d::SimpleLineObject buildFrame(g3d::Renderer& renderer) {
	return {
		g3d::SimpleLineObject::mkBuilder(renderer)
			.setColor({0.1f, 0.1f, 0.1f})
			.primitive<g3d::Cube>({2.0f})
	};
}

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
	g3d::DebugSettings debugSettings;

	unsigned int cells = 40;
	float range = 3.0f;

	ExpressionFunc f;
	g3d::Graph<ExpressionFunc> graph { renderer, f, cells, range };

	auto axes = buildAxes(renderer);
	auto frame = buildFrame(renderer);

	g3d::WithInitial<g3d::Light> light {{
		{0.0f, 0.0f, 1.5f}, // position
		{1.0f, 1.0f, 1.0f}, // color
		1.0f
	}};
	auto lightObject = buildLightObject(renderer);

	g3d::Ui ui;
	ui.addWindow<g3d::CameraWindow>(camController);
	ui.addWindow<g3d::RenderWindow>(renderSettings, light, graph.surfaceMaterial());
	ui.addWindow<g3d::GraphWindow<ExpressionFunc>>(graph);
	ui.addWindow<g3d::DebugWindow>(debugSettings);

	g3d::PrimitiveTest testObject { renderer };

	while (!device.window().shouldClose()) {
		auto frameStartTime = now();

		g3d::Window::pollEvents();
		g3d::imGuiNewFrame();
		g3d::imGuiHandleControlExclusivity(device.window(), camController);

		// UI updates.
		ui.show();

		// Entity and camera updates.
		camController.update();
		g3d::getTransform(lightObject.entity()).translation = light.current.position;
		f.update();

		// Draw the frame.
		renderer.beginFrame(camController.camera(), light);
		if (!renderer.inFrame()) {
			// Indicates we failed to begin the frame for some
			// outside reason - probably window resize
			continue;
		}

		// Update graph GPU resources after beginFrame() to ensure proper
		// synchronization
		graph.update();

		renderer.setMode(g3d::RenderMode::line);
		if (renderSettings.renderFrame) {
			frame.render();
		}

		// TODO This is really dumb but it doesn't look too terrible...
		// Look into using textures for the grid, maybe.
		bool shouldRenderSurfaceGrid = (
			renderSettings.renderGrid &&
			renderSettings.graphRenderMode == g3d::GraphRenderMode::surface
		);
		if (shouldRenderSurfaceGrid) {
			graph.gridTop().render();
			graph.gridBottom().render();
		}

		if (renderSettings.renderNormals) {
			graph.normals().render();
		}

		if (renderSettings.graphRenderMode == g3d::GraphRenderMode::wireframe) {
			graph.wireframe().render();
		}

		if (debugSettings.renderTestObject) {
			testObject.renderLines();
		}

		// TODO: Manually managing all these modes kind of sucks
		renderer.setMode(g3d::RenderMode::triangle);
		if (renderSettings.renderLightObject) {
			lightObject.render();
		}

		renderer.setMode(g3d::RenderMode::litTriangle);
		if (renderSettings.graphRenderMode == g3d::GraphRenderMode::surface) {
			graph.surface().render();
		}

		renderer.setMode(g3d::RenderMode::litTriangleCulled);
		if (renderSettings.renderAxes) {
			axes.render();
		}

		if (debugSettings.renderTestObject) {
			testObject.renderLit();
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
			WINDOW_INITIAL_HEIGHT,
			APPLICATION_ICON
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
