#pragma once

#include "figure/figure.h"
#include "figure/slider.h"
#include "ui/common.h"

namespace g3d {
class SliderElement : public UiElement {
private:
	SliderFigure* _figure;

	float _value = 0.0f;

	// IDs for each of the ImGui widgets
	std::string _varEntryId;
	std::string _valueEntryId;
	std::string _minEntryId;
	std::string _maxEntryId;

	// Updates the variable store with the slider's current value.
	void updateStore();
public:
	SliderElement(
		SliderFigure& figure
	)
	: _figure { &figure },
	  _varEntryId { std::format("##{}varEntry", figure.id()) },
	  _valueEntryId { std::format("##{}valueEntry", figure.id()) },
	  _minEntryId { std::format("##{}minEntry", figure.id()) },
	  _maxEntryId { std::format("##{}maxEntry", figure.id()) }
	{
		updateStore();
	}

	void show() override;
	unsigned int id() const override { return _figure->id(); }
	const std::string& title() const override { return _figure->varString(); }
};

template<>
struct UiElementForFigure<SliderFigure> {
	typedef SliderElement value;
};
}
