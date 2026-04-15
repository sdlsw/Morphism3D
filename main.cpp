#include "vk.h"

#include <iostream>

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

void mainloop(g3d::GraphDevice& device, g3d::Renderer& renderer) {
	g3d::Camera camera {
		{4.0f, 4.0f, 4.0f}, // cam position
		{0.0f, 0.0f, 0.0f}, // coord to look at
		{0.0f, 0.0f, 1.0f}  // up vector
	};

	std::vector<g3d::Vertex> vertices {
		{{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
		{{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},

	};
	std::vector<uint16_t> indices {
		{0, 1, 1, 2, 2, 3, 3, 0}
	};
	g3d::Model model { device, vertices, indices };

	while (!device.window().shouldClose()) {
		g3d::Window::pollEvents();
		renderer.beginFrame(camera);
		model.record(renderer.currentFrameResources());
		renderer.endFrame();
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
		g3d::GraphDevice graphDevice { vkTop, window };
		g3d::Renderer renderer { graphDevice };

		mainloop(graphDevice, renderer);
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
