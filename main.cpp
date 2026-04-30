#include "camera_control.h"
#include "container.h"
#include "vk.h"
#include "window.h"

#include <chrono>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

static const uint32_t WINDOW_INITIAL_WIDTH = 800;
static const uint32_t WINDOW_INITIAL_HEIGHT = 600;
static const std::string APPLICATION_NAME = "graph3d";
static const std::string ENGINE_NAME = APPLICATION_NAME;

static const std::string CAM_MODE_FIXED_NAME = "Fixed";
static const std::string CAM_MODE_FREE_NAME = "Free";

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

static void checkVkResult(VkResult error) {
	if (error == VK_SUCCESS) {
		return;
	}

	std::cerr << "[imgui::vulkan] Error: VkResult = " << error << std::endl;
}

void imGuiInfo(const std::string& message) {
	std::cerr << "[g3d][ImGui] " << message << std::endl;
}

void imGuiInit(g3d::GraphDevice& device, g3d::Renderer& renderer) {
	imGuiInfo("Creating context...");
	ImGui::CreateContext();

	imGuiInfo("Init Glfw backend...");
	ImGui_ImplGlfw_InitForVulkan(device.window().internalWindow(), true);

	imGuiInfo("Init Vulkan backend...");
	ImGui_ImplVulkan_InitInfo initInfo {};
	initInfo.Instance = *device.vkTop().instance();
	initInfo.PhysicalDevice = *device.physicalDevice();
	initInfo.Device = *device.logicalDevice();
	initInfo.QueueFamily = device.queueFamilies().graphicsFamily.value();
	initInfo.Queue = *device.graphicsQueue();
	// Have backend manage its own descriptor pool.
	initInfo.DescriptorPoolSize = 32;
	initInfo.MinImageCount = 2;
	initInfo.ImageCount = renderer.swapchainImageCount();
	initInfo.PipelineInfoMain.RenderPass = *renderer.renderPass();
	initInfo.PipelineInfoMain.Subpass = 0;
	initInfo.CheckVkResultFn = checkVkResult;
	ImGui_ImplVulkan_Init(&initInfo);
}

void imGuiShutdown() {
	imGuiInfo("Shutting down ImplVulkan...");
	ImGui_ImplVulkan_Shutdown();
	imGuiInfo("Shutting down ImplGlfw...");
	ImGui_ImplGlfw_Shutdown();
	imGuiInfo("Destroying context...");
	ImGui::DestroyContext();
}

void imGuiNewFrame() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

// Handles turning on/off input receivers based on what UI elements are being
// used. Must be called every frame.
void imGuiHandleControlExclusivity(g3d::Window& window, g3d::CameraController& camController) {
	ImGuiIO& io = ImGui::GetIO();

	// When ImGui windows are being used, ignore mouse/keyboard inputs to
	// main application.
	window.discardMouseEvents(io.WantCaptureMouse);
	window.discardKeyboardEvents(io.WantCaptureKeyboard);

	// When the camera controller has the mouse captured, do not pass mouse
	// inputs to ImGui.
	if (camController.mouseCaptured()) {
		io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
	} else {
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
	}
}

void imGuiRecord(g3d::RenderContext& ctx) {
	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(drawData, *ctx.frameResources().commandBuffer());
}

// Wrapper class for ImGui setup/teardown to make use of C++'s predictable
// destructor behavior. Cannot be moved or copied.
class ImGuiWrapper {
public:
	ImGuiWrapper(g3d::GraphDevice& device, g3d::Renderer& renderer) {
		imGuiInit(device, renderer);
	}

	ImGuiWrapper(const ImGuiWrapper& other) = delete;
	ImGuiWrapper(ImGuiWrapper&& other) = delete;
	ImGuiWrapper& operator=(const ImGuiWrapper& other) = delete;
	ImGuiWrapper& operator=(ImGuiWrapper&& other) = delete;

	~ImGuiWrapper() {
		imGuiShutdown();
	}
};

std::chrono::time_point<std::chrono::high_resolution_clock> now() {
	return std::chrono::high_resolution_clock::now();
}

float secondsSince(const std::chrono::time_point<std::chrono::high_resolution_clock>& before) {
	auto after = now();
	return std::chrono::duration<float, std::chrono::seconds::period>(after - before).count();
}

// The function to graph.
// TODO Allow the user to specify this
float func(float x, float y) {
	return 0.3f*x*y;
}

template<typename T, typename U>
T lerp2d(const T& ul, const T& ur, const T& dl, const T& dr, U a, U b) {
	T u = glm::mix(ul, ur, a);
	T d = glm::mix(dl, dr, a);

	return glm::mix(u, d, b);
}

// Generates the vertices for a graph model.
// range - The x, y coordinates of the model vertices always range from
//         -1.0f to 1.0f. The range determines how those coordinates map to
//         function input space with a simple relation:
//
//             (model_x, model_y, model_z)*range = (graph_x, graph_y, f(graph_x, graph_y))
//		
//	   graph_x and graph_y will both range from -range to +range.
//
// cells - Determines how many discrete steps to calculate. High values yield
//         higher accuracy, at the cost of additional vertices.
std::vector<g3d::Vertex> generate_func_vertices(float range, unsigned int cells) {
	std::vector<g3d::Vertex> out;

	// Colors for extreme points of graph.
	// nxny - (-range, -range)
	// pxny - (range, -range)
	// nxpy - (-range, range)
	// pxpy - (range, range)
	glm::vec3 nxny {0.141f, 0.706f, 0.322f}; // green
	glm::vec3 pxny {0.988f, 0.804f, 0.000f}; // yellow orange
	glm::vec3 nxpy {0.400f, 0.255f, 0.953f}; // blue violet
	glm::vec3 pxpy {1.000f, 0.000f, 0.000f}; // red

	for (unsigned int ypt = 0; ypt <= cells; ypt++) {
		float lerp_a_y = static_cast<float>(ypt) / static_cast<float>(cells);
		float y = glm::mix(-range, range, lerp_a_y);

		for (unsigned int xpt = 0; xpt <= cells; xpt++) {
			float lerp_a_x = static_cast<float>(xpt) / static_cast<float>(cells);
			float x = glm::mix(-range, range, lerp_a_x);

			out.push_back({
				{x/range, y/range, func(x, y)/range},
				lerp2d(nxny, pxny, nxpy, pxpy, lerp_a_x, lerp_a_y)
			});
		}
	}

	return out;
}

uint16_t idx(unsigned int x, unsigned int y, unsigned int cells) {
	// Note: cells+1 here because there's one more point than the number of
	// cells, for instance:
	//
	//  _ _
	// |_|_|
	// |_|_|
	//
	// 2 cell grid, but 3 points.
	return y*(cells+1) + x;
}

// Generates an index list for a graph. The cells value must match the cells
// value used in generate_func_vertices().
std::vector<uint16_t> generate_func_indices(unsigned int cells) {
	std::vector<uint16_t> out;

	// generate horizontal lines
	for (unsigned int ypt = 0; ypt <= cells; ypt++) {
		for (unsigned int xpt = 0; xpt < cells; xpt++) {
			out.push_back(idx(xpt, ypt, cells));
			out.push_back(idx(xpt+1, ypt, cells));
		}
	}

	// generate vertical lines
	for (unsigned int xpt = 0; xpt <= cells; xpt++) {
		for (unsigned int ypt = 0; ypt < cells; ypt++) {
			out.push_back(idx(xpt, ypt, cells));
			out.push_back(idx(xpt, ypt+1, cells));
		}
	}

	return out;
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

const std::string& camModeName(const g3d::CameraController& controller) {
	if (controller.camera().mode() == g3d::CameraMode::fixedLook) {
		return CAM_MODE_FIXED_NAME;
	} else {
		return CAM_MODE_FREE_NAME;
	}
}

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

void mainloop(g3d::GraphDevice& device, g3d::Renderer& renderer) {
	g3d::CameraController camController {
		{2.5f, -2.5f, 2.5f}, // cam position
		{0.0f, 0.0f, 0.0f},  // initial center
		device.window()
	};

	// Use last 50 frames to calculate average FPS
	g3d::MovingAverage<float> avgFps { 50 };

	g3d::RenderObject graphObject {
		device,
		renderer,
		{
			device,
			generate_func_vertices(3.0f, 20),
			generate_func_indices(20)
		},
		{}
	};
	auto axesObject = createAxes(device, renderer);

	while (!device.window().shouldClose()) {
		auto frameStartTime = now();

		g3d::Window::pollEvents();
		imGuiNewFrame();
		imGuiHandleControlExclusivity(device.window(), camController);

		// Entity and camera updates.
		camController.update();

		// UI updates.
		// For now, demo window is always shown.
		bool showDemoWindow = true;
		if (showDemoWindow) {
			ImGui::ShowDemoWindow(&showDemoWindow);
		}

		// Draw the frame.
		auto& renderContext = renderer.beginFrame(camController.camera());
		if (!renderer.inFrame()) {
			// Indicates we failed to begin the frame for some
			// outside reason - probably window resize
			continue;
		}
		graphObject.record(renderContext);
		axesObject.record(renderContext);
		imGuiRecord(renderContext);
		renderer.endFrame();

		// TODO I don't think this is *quite* accurate since endFrame()
		// is only the submit point for rendering commands; the frame
		// isn't actually finished drawing until a bit later. For now
		// seems fine since there's almost 0 work to do and we're
		// hitting vsync limit.
		avgFps.put(1.0f / secondsSince(frameStartTime));

		device.window().title(std::format(
			"{} | Cam Mode: {} | {:.2f} FPS",
			APPLICATION_NAME,
			camModeName(camController),
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
		ImGuiWrapper imGui { graphDevice, renderer };

		mainloop(graphDevice, renderer);
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
