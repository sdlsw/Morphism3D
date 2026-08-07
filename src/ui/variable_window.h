#pragma once

#include "ui/common.h"
#include "expression.h"
#include "window.h"

namespace g3d {
struct VariableSlider {
	// Tests whether a character can be used as a slider.
	// TODO Factor this out? Will need to change when other types of graphs
	// get added.
	static bool varValid(char c);

	// Text buffer used for variable entry
	static constexpr size_t entrySize = 2;
	std::array<char, entrySize> entryBuffer;

	// Every slider gets a unique numeric ID to differentiate it from the
	// others.
	unsigned int id;
	float value;
	float min;
	float max;
	bool exists = true; // Set to false when the slider is closed

	// Flags for the up/down arrows
	bool upPressed = false;
	bool downPressed = false;

	// For convenience, this variable is stored in the slider,
	// but the actual logic is done in SliderPanel.
	unsigned int height = 0;

	// IDs for each of the ImGui widgets
	std::string varEntryId;
	std::string valueEntryId;
	std::string minEntryId;
	std::string maxEntryId;
	std::string upArrowId;
	std::string downArrowId;

	std::string headerId() { return std::format("{}##{}header", var(), id); }

	VariableSlider(unsigned int id, char var, float value, float min, float max)
	: id { id },
	  varEntryId { std::format("##{}varEntry", id) },
	  valueEntryId { std::format("##{}valueEntry", id) },
	  minEntryId { std::format("##{}minEntry", id) },
	  maxEntryId { std::format("##{}maxEntry", id) },
	  upArrowId { std::format("##{}up", id) },
	  downArrowId { std::format("##{}down", id) },
	  entryBuffer { var, '\0' },
	  value { value },
	  min { min },
	  max { max }
	{}

	VariableSlider(unsigned int id, char var) : VariableSlider(id, var, 1.0f, 0.0f, 3.0f) {}
	VariableSlider(unsigned int id) : VariableSlider(id, '\0') {}

	// Show logic split into two functions for imgui group stuff in SliderPanel

	// Returns true if the body should be shown.
	bool showHeader();

	// Returns true if the variable slider was changed in any way that
	// requires an update of the associated variable store.
	bool showBody();

	// Gets the variable this slider is connected to
	char var() const;
};

class SliderPanel {
private:
	Window* _window;
	VariableStore* _vars;

	std::unordered_map<unsigned int, VariableSlider> _sliders;
	std::vector<unsigned int> _sliderOrder;
	unsigned int _nextSliderId = 0;

	size_t _rearrangeFrom = 0;
	size_t _rearrangeTo = 0;
	bool _rearrange = false;

	// Need to do high level slider show logic here since drag/drop
	// interacts with _sliderOrder.
	void showSlider(size_t i);

	// Shows all sliders, handles delete/variable update logic
	void showSliders();

	// Utility function. Gets the i'th slider based on order in the UI
	VariableSlider& getSlider(size_t i);

	// Returns true if this panel has a slider with the given character
	// defined.
	bool hasSlider(char c);

	// Adds a slider. Sliders are removed in the UI through an X on their
	// header bar.
	void addSlider(char c);

	// Finds an available variable that doesn't already have a slider.
	// Returns '\0' on failure.
	char findFirstAvailableVar();
public:
	SliderPanel(Window& window, VariableStore& vars)
	: _window { &window }, _vars { &vars } {}
	void show();
};

class VariableWindow : public UiWindow {
private:
	const std::string _title { "Variables" };
	SliderPanel _sliders;
public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	VariableWindow(VariableStore& vars, Window& window)
	: _sliders { window, vars }
	{}
};
}
