#include "application.h"

#include <iostream>

static const std::string APPNAME = APPLICATION_NAME;
static const std::string ENGINE_NAME = APPLICATION_NAME;

g3d::VkTop init_top() {
	vk::ApplicationInfo appInfo {
		.pApplicationName = APPNAME.c_str(),
		.applicationVersion = VULKAN_VERSION,
		.pEngineName = ENGINE_NAME.c_str(),
		.engineVersion = VULKAN_VERSION,
		.apiVersion = VK_API_VERSION_1_1
	};
	g3d::VkTop vk_top { appInfo };
	return vk_top;
}

int main() {
	try {
		// FIXME: This should happen automatically on first window
		// creation, but that doesn't seem to work.
		g3d::Window::initWindowingSystem();

		auto vkTop = init_top();
		g3d::Application app { vkTop };
		app.mainloop();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
