#include "ui.h"

#include "vk/constants.h"

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
	initInfo.PipelineInfoMain.MSAASamples = static_cast<VkSampleCountFlagBits>(g3d::MSAA_SAMPLES);
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

static const std::string UNKNOWN_MODE_NAME = "???";

static const std::string CAM_MODE_FIXED_NAME = "Fixed";
static const std::string CAM_MODE_FREE_NAME = "Free";

static const std::string& camModeName(const g3d::CameraMode& mode) {
	switch (mode) {
		case g3d::CameraMode::fixedLook:
			return CAM_MODE_FIXED_NAME;
		case g3d::CameraMode::forward:
			return CAM_MODE_FREE_NAME;
		default:
			return UNKNOWN_MODE_NAME;
	}
}

static const std::string RENDER_MODE_SURFACE_NAME = "Surface";
static const std::string RENDER_MODE_WIREFRAME_NAME = "Wireframe";
static const std::string RENDER_MODE_NONE_NAME = "None";

static const std::string& renderModeName(const g3d::GraphRenderMode& mode) {
	switch (mode) {
		case g3d::GraphRenderMode::surface:
			return RENDER_MODE_SURFACE_NAME;
		case g3d::GraphRenderMode::wireframe:
			return RENDER_MODE_WIREFRAME_NAME;
		case g3d::GraphRenderMode::none:
			return RENDER_MODE_NONE_NAME;
		default:
			return UNKNOWN_MODE_NAME;
	}
}

static const std::string PERSPECTIVE_FIXED_IN_FREE_TOOLTIP = "This setting is fixed to 1 in Free mode.";
static const std::string POSITION_DISABLED_FIXED_TOOLTIP = "Position is set automatically in Fixed mode.";

static const std::string SHOW_GRID_FIXED_NOT_SURFACE_TOOLTIP = (
	"This setting cannot be changed outside Surface rendering mode."
);

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

void imGuiRecord(Renderer& renderer) {
	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(drawData, *renderer.currentCommandBuffer());
}

template<>
void resettableSlider<float, float>(
	const std::string& label,
	float* setting,
	const float& _default,
	float min,
	float max
) {
	ImGui::SliderFloat(label.c_str(), setting, min, max);
	resetButton(label, setting, _default);
}

template<>
void resettableSlider<glm::vec3, float>(
	const std::string& label,
	glm::vec3* setting,
	const glm::vec3& _default,
	float min,
	float max
) {
	ImGui::SliderFloat3(label.c_str(), glm::value_ptr(*setting), min, max);
	resetButton(label, setting, _default);
}

template<>
void resettableDrag<glm::vec3, float>(
	const std::string& label,
	glm::vec3* setting,
	const glm::vec3& _default,
	float inc
) {
	ImGui::DragFloat3(label.c_str(), glm::value_ptr(*setting), inc);
	resetButton(label, setting, _default);
}

void CameraWindow::settingSlider(
	const std::string& label,
	float* setting,
	float defaultValue
) {
	resettableSlider(label, setting, defaultValue, defaultValue / 5.0f, defaultValue * 3.0f);
}

void CameraWindow::projectionSlider() {
	bool disabled = _camController->mode() == CameraMode::forward;

	float fixed = Camera::defaultProjectionMix;
	float* setting = disabled ? &fixed : &_camController->camera().projectionMix;

	ImGui::BeginDisabled(disabled);
	// TODO Should use BeginGroup/EndGroup for this, but it messes with
	// the spacing of the Reset button. Will figure it out later.
	ImGui::SliderFloat("Perspective", setting, 0.0f, 1.0f);
	if (disabled) ImGui::SetItemTooltip(PERSPECTIVE_FIXED_IN_FREE_TOOLTIP.c_str());
	resetButton("Perspective", setting, Camera::defaultProjectionMix);
	if (disabled) ImGui::SetItemTooltip(PERSPECTIVE_FIXED_IN_FREE_TOOLTIP.c_str());
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

void CameraWindow::transformDrags() {
	bool positionDisabled = _camController->mode() == CameraMode::fixedLook;

	ImGui::BeginDisabled(positionDisabled);
	ImGui::DragFloat3(
		"Position",
		glm::value_ptr(_camController->camera().position),
		0.025f,
		-FLT_MAX, FLT_MAX
	);
	if (positionDisabled) {
		ImGui::SetItemTooltip(POSITION_DISABLED_FIXED_TOOLTIP.c_str());
	}
	ImGui::EndDisabled();

	// Note: No need to limit angle here since the camera will apply modulo
	// 2pi on update.
	ImGui::DragFloat2(
		"Angles",
		glm::value_ptr(_camController->camera().angles),
		0.025f,
		-FLT_MAX, FLT_MAX
	);
}

void CameraWindow::alignButtons() {
	auto fullCircle = 2.0f * glm::pi<float>();
	std::vector<std::pair<std::string, glm::vec2>> axes {
		{"+X", {fullCircle * 0.75f, 0.0f}},
		{"-X", {fullCircle * 0.25f, 0.0f}},
		{"+Y", {0.0f, 0.0f}},
		{"-Y", {fullCircle * 0.5f, 0.0f}},
		{"+Z", {fullCircle * 0.5f, fullCircle * 0.25f}},
		{"-Z", {0.0f, fullCircle * 0.75f}}
	};

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Align");
	for (const auto& [label, angles] : axes) {
		ImGui::SameLine();
		if (ImGui::Button(label.c_str())) {
			_camController->camera().angles = angles;
		}
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

void CameraWindow::drawUi() {
	ImGui::SeparatorText("Settings");
	settingSliders();
	projectionSlider();
	modeSlider();
	if (ImGui::Button("Reset View")) {
		_camController->reset();
	}

	ImGui::SeparatorText("Transform");
	transformDrags();
	alignButtons();

	ImGui::SeparatorText("Info");
	controlTutorial();
}

void RenderWindow::showGridToggle() {
	bool disabled = _renderSettings->graphRenderMode != GraphRenderMode::surface;
	bool forceSetting = _renderSettings->graphRenderMode == GraphRenderMode::wireframe;
	bool* setting = disabled ? &forceSetting : &_renderSettings->renderGrid;

	ImGui::BeginDisabled(disabled);
	ImGui::Checkbox("Show Grid", setting);
	if (disabled) ImGui::SetItemTooltip(SHOW_GRID_FIXED_NOT_SURFACE_TOOLTIP.c_str());
	ImGui::EndDisabled();
}

void RenderWindow::basicSettingsSection() {
	ImGui::SeparatorText("Basic Settings");

	ImGui::SliderInt(
		"Render Mode",
		reinterpret_cast<int*>(&_renderSettings->graphRenderMode),
		0,
		GraphRenderModeCount-1,
		renderModeName(_renderSettings->graphRenderMode).c_str(),
		ImGuiSliderFlags_NoInput
	);

	ImGui::Checkbox("Show Axes", &_renderSettings->renderAxes);
	showGridToggle();
	ImGui::Checkbox("Show Frame", &_renderSettings->renderFrame);
	ImGui::Checkbox("Show Normals", &_renderSettings->renderNormals);
	ImGui::Checkbox("Show Light Position", &_renderSettings->renderLightObject);
}

void RenderWindow::lightSection() {
	ImGui::SeparatorText("Light");
	resettableSlider("Mix",
		&_light->current.mix,
		_light->initial().mix,
		0.0f, 1.0f
	);
	resettableSlider("Color",
		&_light->current.color,
		_light->initial().color,
		0.0f, 1.0f
	);
	resettableDrag("Position",
		&_light->current.position,
		_light->initial().position,
		0.025f
	);
	resetAllButton("Light", *_light);
}

void RenderWindow::materialSection() {
	ImGui::SeparatorText("Material");
	resettableSlider("Ambient",
		&_material->current.ambient,
		_material->initial().ambient,
		0.0f, 1.0f
	);
	resettableSlider("Diffuse",
		&_material->current.diffuse,
		_material->initial().diffuse,
		0.0f, 1.0f
	);
	resettableSlider("Specular",
		&_material->current.specular,
		_material->initial().specular,
		0.0f, 1.0f
	);
	resettableSlider("Shine",
		&_material->current.shine,
		_material->initial().shine,
		1.0f, 64.0f
	);
	resetAllButton("Material", *_material);
}

void RenderWindow::drawUi() {
	basicSettingsSection();
	lightSection();
	materialSection();
}

void DebugWindow::drawUi() {
	ImGui::Checkbox("Show Test Object", &_debugSettings->renderTestObject);
}

void UiWindow::show() {
	if (!open) return;

	ImGui::Begin(title().c_str(), &open);
	ImGui::PushItemWidth(200.0f);
	drawUi();
	ImGui::PopItemWidth();
	ImGui::End();
}

void Ui::show() {
	mainMenuBar();

	if (showDemoWindow) {
		ImGui::ShowDemoWindow(&showDemoWindow);
	}

	for (auto& window : _windows) {
		window.get()->show();
	}
}

void Ui::mainMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		windowMenu();
		ImGui::EndMainMenuBar();
	}
}

void Ui::windowMenu() {
	if (ImGui::BeginMenu("Window")) {
		for (auto& window : _windows) {
			auto* ptr = window.get();
			ImGui::MenuItem(ptr->title().c_str(), nullptr, &ptr->open);
		}
		ImGui::MenuItem("ImGui Demo", nullptr, &showDemoWindow);
		ImGui::EndMenu();
	}
}
}
