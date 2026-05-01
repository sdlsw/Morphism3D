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

static const std::string CAM_MODE_FIXED_NAME = "Fixed";
static const std::string CAM_MODE_FREE_NAME = "Free";
static const std::string CAM_MODE_UNKNOWN_NAME = "???";

static const std::string& camModeName(const g3d::CameraMode& mode) {
	switch (mode) {
		case g3d::CameraMode::fixedLook:
			return CAM_MODE_FIXED_NAME;
		case g3d::CameraMode::forward:
			return CAM_MODE_FREE_NAME;
		default:
			return CAM_MODE_UNKNOWN_NAME;
	}
}

static const std::string SETTING_IGNORED_FREE_TOOLTIP = "This setting is ignored in freecam mode.";

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

void CameraWindow::resetButton(
	const std::string& resetFor,
	float* setting,
	float defaultValue
) {
	ImGui::SameLine(ImGui::GetWindowWidth()-50);
	if (ImGui::Button(std::format("Reset##{}", resetFor).c_str())) {
		*setting = defaultValue;
	}
}

void CameraWindow::settingSlider(
	const std::string& label,
	float* setting,
	float defaultValue,
	float minValue,
	float maxValue
) {
	ImGui::SliderFloat(label.c_str(), setting, minValue, maxValue);
	resetButton(label, setting, defaultValue);
}

void CameraWindow::settingSlider(
	const std::string& label,
	float* setting,
	float defaultValue
) {
	settingSlider(label, setting, defaultValue, defaultValue / 5.0f, defaultValue * 3.0f);
}

void CameraWindow::projectionSlider() {
	bool disabled = _camController->mode() == CameraMode::forward;
	float* setting = &_camController->camera().projectionMix;

	ImGui::BeginDisabled(disabled);
	ImGui::SliderFloat( "Projection", setting, 0.0f, 1.0f);
	if (disabled) ImGui::SetItemTooltip(SETTING_IGNORED_FREE_TOOLTIP);
	resetButton("Projection", setting, Camera::defaultProjectionMix);
	if (disabled) ImGui::SetItemTooltip(SETTING_IGNORED_FREE_TOOLTIP);
	ImGui::EndDisabled();
}

void CameraWindow::settingSliders() {
	settingSlider(
		"Sensitivity",
		&_camController->sensitivity,
		CameraController::defaultSensitivity
	);
	settingSlider(
		"Freecam Speed",
		&_camController->moveSpeed,
		CameraController::defaultMoveSpeed
	);
	settingSlider(
		"Scroll Speed",
		&_camController->scrollSpeed,
		CameraController::defaultScrollSpeed
	);
}

void CameraWindow::modeSlider() {
	int currentMode = static_cast<int>(_camController->mode());
	int sliderMode = currentMode;

	ImGui::SliderInt(
		"Mode",
		&sliderMode,
		0,
		CameraModeCount-1,
		camModeName(_camController->mode()).c_str(),
		ImGuiSliderFlags_NoInput
	);

	if (sliderMode != currentMode) {
		_camController->mode(static_cast<CameraMode>(sliderMode));
	}
}

void CameraWindow::controlTutorial() {
	if (ImGui::TreeNode("Controls")) {
		ImGui::Text("Click outside a window to capture the cursor.");
		ImGui::Text("ESC - Free cursor");
		ImGui::Text("E - Toggle camera mode");
		ImGui::Text("R - Reset camera view");

		if (_camController->mode() == CameraMode::fixedLook) {
			ImGui::Text("Scroll wheel - Up zooms in, down zooms out");
			ImGui::Text("Move mouse - Rotate around graph");
		} else {
			ImGui::Text("WASD - Standard first person controls");
			ImGui::Text("Space - Move up");
			ImGui::Text("Left Shift - Move down");
			ImGui::Text("Move mouse - Look around");
		}

		ImGui::TreePop();
	}
}

void CameraWindow::show() {
	if (!open) return;

	ImGui::Begin("Camera", &open);
	settingSliders();
	projectionSlider();
	modeSlider();
	if (ImGui::Button("Reset View")) {
		_camController->reset();
	}
	controlTutorial();
	ImGui::End();
}

void Ui::show() {
	mainMenuBar();

	if (showDemoWindow) {
		ImGui::ShowDemoWindow(&showDemoWindow);
	}

	_cameraWindow.show();
}

void Ui::mainMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		windowMenu();
		ImGui::EndMainMenuBar();
	}
}

void Ui::windowMenu() {
	if (ImGui::BeginMenu("Window")) {
		ImGui::MenuItem("Camera", nullptr, &_cameraWindow.open);
		ImGui::MenuItem("ImGui Demo", nullptr, &showDemoWindow);
		ImGui::EndMenu();
	}
}
}
