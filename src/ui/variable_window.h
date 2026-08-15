#pragma once

#include "ui/common.h"
#include "expression.h"
#include "window.h"

namespace g3d {
struct SliderRemovedEvent {
	unsigned int id;
};

class SliderElement : public UiElement {
private:
	VariableStore* _variableStore;

	class _SliderRemovedHandler : public EventHandler<SliderRemovedEvent> {
	public:
		SliderElement* _this;

		void handle(const SliderRemovedEvent& e) override {
			std::cout <<
				"Handle SliderRemovedEvent: " <<
				"e.id=" << e.id <<
				" _this->_id=" << _this->_id <<
				std::endl;
			if (e.id != _this->_id) return;

			_this->_value = 0.0f;
			_this->updateStore();
		}

		_SliderRemovedHandler(SliderElement* elem) : _this { elem } {}
	};

	// Text buffer used for variable entry
	static constexpr size_t _entrySize = 2;
	std::string _entryBuffer;

	// Every slider gets a unique numeric ID to differentiate it from the
	// others.
	unsigned int _id;
	float _value;
	float _min;
	float _max;

	// IDs for each of the ImGui widgets
	std::string _varEntryId;
	std::string _valueEntryId;
	std::string _minEntryId;
	std::string _maxEntryId;

	// Updates the variable store with the slider's current value.
	void updateStore();

	_SliderRemovedHandler _sliderRemovedHandler { this };
public:
	SliderElement(
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
	  _entryBuffer { var, '\0' },
	  _value { value },
	  _min { min },
	  _max { max }
	{
		std::string msg = std::format(
			"Created new SliderElement: id {} var {} val {}",
			id, _entryBuffer[0], value
		);
		std::cerr << msg << std::endl;
		variableStore.eventRouter().addHandler(_sliderRemovedHandler);
		updateStore();
	}

	SliderElement(
		VariableStore& variableStore,
		unsigned int id,
		char var
	) : SliderElement(variableStore, id, var, 1.0f, 0.0f, 3.0f) {}

	SliderElement(
		VariableStore& variableStore,
		unsigned int id
	) : SliderElement(variableStore, id, '\0') {}

	// FIXME gross, but required in order to put correct `this` pointer in
	// _sliderRemovedHandler (default move constructor copies it from
	// source object)
	SliderElement(SliderElement&& other)
	: _variableStore { other._variableStore },
	  _id { other._id },
	  _varEntryId { std::move(other._varEntryId) },
	  _valueEntryId { std::move(other._valueEntryId) },
	  _minEntryId { std::move(other._minEntryId) },
	  _maxEntryId { std::move(other._maxEntryId) },
	  _entryBuffer { std::move(other._entryBuffer) },
	  _value { other._value },
	  _min { other._min },
	  _max { other._max },
	  // Can't just be _sliderRemovedHandler { this } since event handler
	  // move constructor updates the id->pointer maps in EventRouter
	  _sliderRemovedHandler { std::move(other._sliderRemovedHandler) }
	{
		_sliderRemovedHandler._this = this;
	}

	// Tests whether a character can be used as a slider.
	// TODO Factor this out? Will need to change when other types of graphs
	// get added.
	static bool varValid(char c);

	void show() override;
	unsigned int id() const override { return _id; }
	const std::string& title() const override { return _entryBuffer; }

	char var() const;
};

class VariableWindow : public UiWindow {
private:
	const std::string _title { "Variables" };
	RearrangeablePanel _panel;

	VariableStore* _vars;

	std::unordered_map<unsigned int, SliderElement> _sliders;
	unsigned int _nextSliderId = 0;

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
	const std::string& title() const override { return _title; }
	void drawUi() override;

	VariableWindow(VariableStore& vars, Window& window)
	: _panel { window },
	  _vars { &vars }
	{}
};
}
