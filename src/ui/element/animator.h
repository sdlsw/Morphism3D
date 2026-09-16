#pragma once

#include "figure/figure.h"
#include "figure/animator.h"

namespace g3d {
class AnimatorElement : public UiElement {
private:
	AnimatorFigure* _figure;

	DiscriminatedStringMap _guiIds;

public:
	AnimatorElement(
		AnimatorFigure& figure
	)
	: _figure { &figure },
	  _guiIds { std::format("##{}", figure.id()) }
	{}

	void show() override;
	unsigned int id() const override { return _figure->id(); }
	const std::string& title() const override { return _figure->varRange().varString; }
};

template<>
struct UiElementForFigure<AnimatorFigure> {
	typedef AnimatorElement value;
};
}
