#pragma once

#include "figure/figure.h"
#include "figure/slider.h"

namespace g3d {
class SliderElement : public UiElement {
private:
	SliderFigure* _figure;
	std::string _title;

	float _value = 0.0f;

	DiscriminatedStringMap _guiIds;

	// Updates the variable store with the slider's current value.
	void updateStore();
	void updateTitle();
public:
	SliderElement(
		SliderFigure& figure
	)
	: _figure { &figure },
	  _guiIds { std::format("{}", figure.id()) }
	{
		updateStore();
		updateTitle();
	}

	void show() override;
	unsigned int id() const override { return _figure->id(); }
	const std::string& title() const override { return _title; }
};

template<>
struct UiElementForFigure<SliderFigure> {
	typedef SliderElement value;
};
}
