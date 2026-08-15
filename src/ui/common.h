#pragma once
#include "global_defines.h"

#include "container.h"
#include "window.h"

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

class UiElement {
private:
	const std::string unknownTitle { "unknown" };
public:
	bool shouldShow = true;
	virtual const std::string& title() const { return unknownTitle; }
	virtual unsigned int id() const { return 0; }
	virtual void show() {}
};

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

class RearrangeFrame {
	UiElement* _elem;

	// Flags for the up/down arrows
	bool _upPressed = false;
	bool _downPressed = false;

	std::string _upArrowId;
	std::string _downArrowId;

	std::string headerId() { return std::format("{}##{}header", _elem->title(), _elem->id()); }

public:
	bool exists = true; // Set to false when the element is closed

	RearrangeFrame(UiElement& elem)
	: _elem { &elem },
	  _upArrowId { std::format("##{}up", elem.id()) },
	  _downArrowId { std::format("##{}down", elem.id()) }
	{}

	bool showHeader();
	void showBody();

	const std::string& title() const { return _elem->title(); }
	UiElement& elem() { return *_elem; }
	bool downPressed() const { return _downPressed; }
	bool upPressed() const { return _upPressed; }

	std::string fromTooltip() const { return std::format("Moving {}", _elem->title()); }
	std::string toTooltip() const { return std::format("Move to {}", _elem->title()); }
};

class RearrangeablePanel {
private:
	// Initialized in source file
	static unsigned int _nextPanelId;

	Window* _window;
	std::string _dragDropTarget;
	unsigned int _id;

	std::unordered_map<unsigned int, RearrangeFrame> _frames;
	// Heights of each frame, indexed by ID. Used for moving cursor on
	// up/down arrow presses
	std::unordered_map<unsigned int, unsigned int> _heights;
	std::vector<unsigned int> _frameOrder;
	unsigned int _nextSliderId = 0;

	size_t _rearrangeFrom = 0;
	size_t _rearrangeTo = 0;
	bool _rearrange = false;

	// Need to do high level frame show logic here since drag/drop
	// interacts with _frameOrder.
	void showFrame(size_t i);

	// Shows all sliders, handles delete logic and performs rearrangement
	// if _rearrange is true
	void showFrames();

	// Utility function. Gets the i'th frame based on order in the UI
	RearrangeFrame& getFrame(size_t i);
public:
	RearrangeablePanel(Window& window)
	: _window { &window },
	  _id { _nextPanelId++ },
	  _dragDropTarget { std::format("REARRANGE_PANEL{}", _id) }
	{}

	void show();

	void addFrame(UiElement& elem);
	void removeAllFrames();
};
}
