#pragma once

#include "expression.h"
#include "figure/common.h"
#include "figure/iface.h"
#include "temporal.h"
#include "ui/common_expression.h"

#include "global_defines.h"
#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>

#include <string>

namespace g3d {
class AnimatorFigure : public MathFigure {
private:
	VariableRange _varRange;

	TimePoint _startTime;

	void handleFigureRemoved(const FigureRemovedEvent& e);

	CompoundMethodEventHandler<AnimatorFigure,
		FigureRemovedEvent
	> _eventHandlers { this,
		std::mem_fn(handleFigureRemoved)
	};

	unsigned int _id;
public:
	AnimatorFigure(unsigned int id, VariableStore& variableStore, char v)
	: _id { id },
	  _varRange { variableStore, v, 0.0f, glm::pi<float>() },
	  _startTime { now() }
	{
		_eventHandlers.addToRouter(variableStore.eventRouter());
	}

	AnimatorFigure(AnimatorFigure&& other)
	: _id { other._id },
	  _varRange { other._varRange },
	  _eventHandlers { std::move(other._eventHandlers) },
	  _startTime { other._startTime }
	{
		_eventHandlers.updateThis(this);
	}

	unsigned int id() const override { return _id; }

	void reset();
	float currentTime();
	void update() override;

	VariableRange& varRange() { return _varRange; }
};
}
