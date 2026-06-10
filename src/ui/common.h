#pragma once
#include "global_defines.h"

#include "container.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>

#include <format>
#include <string>

namespace g3d {
constexpr char UI_UNKNOWN_MODE_NAME[] { "???" };

template<typename T>
void resetAllButton(const std::string& name, WithInitial<T>& withInitial) {
	if(ImGui::Button(std::format("Reset All##{}", name).c_str())) {
		withInitial.reset();
	}
}

template<typename T>
void resetButton(const std::string& label, T* setting, T _default) {
	ImGui::SameLine(ImGui::GetWindowWidth()-50);
	if (ImGui::Button(std::format("Reset##{}", label).c_str())) {
		*setting = _default;
	}
}

template<typename T, typename U>
void resettableSlider(const std::string& label, T* setting, const T& _default, U min, U max);

template<typename T, typename U>
void resettableDrag(const std::string& label, T* setting, const T& _default, U inc);

// Base class for all UI windows
class UiWindow {
private:
	const std::string unknownTitle { "unknown" };
public:
	bool open = true;
	void show();
	virtual const std::string& title() const { return unknownTitle; }
	virtual void drawUi() {}
};
}
