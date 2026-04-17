#include "vk.h"
#include "window.h"

#include <chrono>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

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

class TestObject {
private:
	// Radians per second
	float _rotationSpeed;
	g3d::RenderObject _obj;

public:
	TestObject(
		g3d::GraphDevice& device,
		g3d::Renderer& renderer,
		float rotationSpeed,
		float z
	) :
		_rotationSpeed {rotationSpeed},
		_obj {
			device,
			renderer,
			{
				device,
				{
					{{1.0f, 1.0f, z}, {1.0f, 0.0f, 0.0f}},
					{{-1.0f, 1.0f, z}, {0.0f, 1.0f, 0.0f}},
					{{-1.0f, -1.0f, z}, {0.0f, 0.0f, 1.0f}},
					{{1.0f, -1.0f, z}, {1.0f, 1.0f, 1.0f}}
				},
				{0, 1, 1, 2, 2, 3, 3, 0}
			}
		} {}

	void updateAndRender(g3d::RenderContext& ctx) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		_obj.transform(glm::rotate(
			glm::mat4(1.0f),
			time * _rotationSpeed,
			glm::vec3(0.0f, 0.0f, 1.0f)
		));
		_obj.update(ctx);
		_obj.record(ctx);
	}
};

void mainloop(g3d::GraphDevice& device, g3d::Renderer& renderer) {
	g3d::Camera camera {
		{4.0f, 4.0f, 4.0f}, // cam position
		{0.0f, 0.0f, 0.0f}, // coord to look at
		{0.0f, 0.0f, 1.0f}  // up vector
	};

	TestObject obj1 { device, renderer, glm::radians(90.0f), 0.0f };
	TestObject obj2 { device, renderer, glm::radians(-120.0f), 1.0f };

	while (!device.window().shouldClose()) {
		g3d::Window::pollEvents();

		auto& renderContext = renderer.beginFrame(camera);
		obj1.updateAndRender(renderContext);
		obj2.updateAndRender(renderContext);
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
