#pragma once

#include "camera_control.h"
#include "vk/renderer.h"

#include <imgui.h>

#include <string>

namespace g3d {
// Wrapper class for ImGui setup/teardown to make use of C++'s predictable
// destructor behavior. Cannot be moved or copied.
class ImGuiWrapper {
public:
	ImGuiWrapper(const ImGuiWrapper& other) = delete;
	ImGuiWrapper(ImGuiWrapper&& other) = delete;
	ImGuiWrapper& operator=(const ImGuiWrapper& other) = delete;
	ImGuiWrapper& operator=(ImGuiWrapper&& other) = delete;

	ImGuiWrapper(GraphDevice& device, Renderer& renderer);
	~ImGuiWrapper();
};

void imGuiInfo(const std::string& message);
void imGuiNewFrame();
void imGuiRecord(Renderer& renderer);
void imGuiHandleControlExclusivity(Window& window, CameraController& camController);
}
