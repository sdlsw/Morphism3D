#include "ui.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

static void checkVkResult(VkResult error) {
	if (error == VK_SUCCESS) {
		return;
	}

	std::cerr << "[g3d][ImGui][Vulkan] Error: VkResult = " << error << std::endl;
}

static void init(g3d::GraphDevice& device, g3d::Renderer& renderer) {
	g3d::imGuiInfo("Creating context...");
	ImGui::CreateContext();

	g3d::imGuiInfo("Init Glfw backend...");
	ImGui_ImplGlfw_InitForVulkan(device.window().internalWindow(), true);

	g3d::imGuiInfo("Init Vulkan backend...");
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

static void shutdown() {
	g3d::imGuiInfo("Shutting down ImplVulkan...");
	ImGui_ImplVulkan_Shutdown();
	g3d::imGuiInfo("Shutting down ImplGlfw...");
	ImGui_ImplGlfw_Shutdown();
	g3d::imGuiInfo("Destroying context...");
	ImGui::DestroyContext();
}

namespace g3d {
ImGuiWrapper::ImGuiWrapper(GraphDevice& device, Renderer& renderer) {
	init(device, renderer);
}

ImGuiWrapper::~ImGuiWrapper() {
	shutdown();
}

void imGuiInfo(const std::string& message) {
	std::cerr << "[g3d][ImGui] " << message << std::endl;
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

void imGuiRecord(g3d::RenderContext& ctx) {
	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(drawData, *ctx.frameResources().commandBuffer());
}
}
