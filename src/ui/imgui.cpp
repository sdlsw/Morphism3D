#include "ui/imgui.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

static g3d::Logger _logger { "imgui" };

static void checkVkResult(VkResult error) {
	if (error == VK_SUCCESS) {
		return;
	}

	_logger.error("Vulkan Error: VkResult = {}", std::to_underlying(error));
}

static void init(g3d::GraphDevice& device, g3d::Renderer& renderer) {
	_logger.info("Creating context...");
	ImGui::CreateContext();

	_logger.info("Init Glfw backend...");
	ImGui_ImplGlfw_InitForVulkan(device.window().internalWindow(), true);

	_logger.info("Init Vulkan backend...");
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
	initInfo.PipelineInfoMain.MSAASamples = static_cast<VkSampleCountFlagBits>(g3d::MSAA_SAMPLES);
	initInfo.CheckVkResultFn = checkVkResult;
	ImGui_ImplVulkan_Init(&initInfo);
}

static void shutdown() {
	_logger.info("Shutting down ImplVulkan...");
	ImGui_ImplVulkan_Shutdown();
	_logger.info("Shutting down ImplGlfw...");
	ImGui_ImplGlfw_Shutdown();
	_logger.info("Destroying context...");
	ImGui::DestroyContext();
}

namespace g3d {
ImGuiWrapper::ImGuiWrapper(GraphDevice& device, Renderer& renderer) {
	init(device, renderer);
}

ImGuiWrapper::~ImGuiWrapper() {
	shutdown();
}

void imGuiNewFrame() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

// Handles turning on/off input receivers based on what UI elements are being
// used. Must be called every frame.
void imGuiHandleControlExclusivity(Window& window, CameraController& camController) {
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

void imGuiRecord(Renderer& renderer) {
	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(drawData, *renderer.currentCommandBuffer());
}
}
