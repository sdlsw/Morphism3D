#pragma once

#include "ui/common.h"
#include "expression.h"
#include "window.h"

namespace g3d {
class VariableSlider {
private:
	VariableStore* _variableStore;

	// Text buffer used for variable entry
	static constexpr size_t _entrySize = 2;
	std::array<char, _entrySize> _entryBuffer;

	// Every slider gets a unique numeric ID to differentiate it from the
	// others.
	unsigned int _id;
	float _value;
	float _min;
	float _max;

	// Flags for the up/down arrows
	bool _upPressed = false;
	bool _downPressed = false;

	// IDs for each of the ImGui widgets
	std::string _varEntryId;
	std::string _valueEntryId;
	std::string _minEntryId;
	std::string _maxEntryId;
	std::string _upArrowId;
	std::string _downArrowId;

	std::string headerId() { return std::format("{}##{}header", var(), _id); }

	// Updates the variable store with the slider's current value.
	void updateStore();

public:
	bool exists = true; // Set to false when the slider is closed

	VariableSlider(
		VariableStore& variableStore,
		unsigned int id,
		char var,
		float value,
		float min,
		float max
	)
	: _variableStore { &variableStore },
	  _id { id },
	  _varEntryId { std::format("##{}varEntry", id) },
	  _valueEntryId { std::format("##{}valueEntry", id) },
	  _minEntryId { std::format("##{}minEntry", id) },
	  _maxEntryId { std::format("##{}maxEntry", id) },
	  _upArrowId { std::format("##{}up", id) },
	  _downArrowId { std::format("##{}down", id) },
	  _entryBuffer { var, '\0' },
	  _value { value },
	  _min { min },
	  _max { max }
	{}

	VariableSlider(
		VariableStore& variableStore,
		unsigned int id,
		char var
	) : VariableSlider(variableStore, id, var, 1.0f, 0.0f, 3.0f) {}

	VariableSlider(
		VariableStore& variableStore,
		unsigned int id
	) : VariableSlider(variableStore, id, '\0') {}

	// Tests whether a character can be used as a slider.
	// TODO Factor this out? Will need to change when other types of graphs
	// get added.
	static bool varValid(char c);

	// Show logic split into two functions for imgui group stuff in SliderPanel.
	// showHeader() returns true if the body should be shown
	bool showHeader();
	void showBody();

	// Called when the slider is added.
	void onAdd();

	// Called when the slider is removed.
	void onRemove();

	unsigned int id() const { return _id; }
	bool downPressed() const { return _downPressed; }
	bool upPressed() const { return _upPressed; }

	// Gets the variable this slider is connected to
	char var() const;
};

class SliderPanel {
private:
	Window* _window;
	VariableStore* _vars;

	std::unordered_map<unsigned int, VariableSlider> _sliders;
	// Heights of each slider, indexed by slider ID
	std::unordered_map<unsigned int, unsigned int> _heights;
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
