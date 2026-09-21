#pragma once

#include "figure/figure.h"
#include "figure/animator.h"

namespace g3d {
class AnimatorElement : public UiElement {
private:
	AnimatorFigure* _figure;
	std::string _title;

	DiscriminatedStringMap _guiIds;

	void updateTitle();
public:
	AnimatorElement(
		AnimatorFigure& figure
	)
	: _figure { &figure },
	  _guiIds { std::format("##{}", figure.id()) }
	{
		updateTitle();
	}

	void show() override;
	unsigned int id() const override { return _figure->id(); }
	const std::string& title() const override { return _title; }
};

template<>
struct UiElementForFigure<AnimatorFigure> {
	typedef AnimatorElement value;
};
}
