#pragma once

#include "expression.h"
#include "temporal.h"

namespace g3d {
class Function {
private:
	void handleVariableChanged(const VariableChangedEvent& e);

	CompoundMethodEventHandler<Function,
		VariableChangedEvent
	> _eventHandlers { this,
		std::mem_fn(handleVariableChanged)
	};

	TokenRegistry _tokenRegistry;
	VariableStore* _vars;
	TimePoint _startTime;
	std::unique_ptr<ParseNode> _parsedExpression;
	bool _animated = false;
	bool _updated = false;
public:
	Function(VariableStore& vars)
	: _vars { &vars },
	  _tokenRegistry { makeTokenRegistry() },
	  _startTime { now() }
	{
		_eventHandlers.addToRouter(vars.eventRouter());
	}

	Function(Function&& other)
	: _vars { other._vars },
	  _tokenRegistry { std::move(other._tokenRegistry) },
	  _startTime { other._startTime },
	  _eventHandlers { std::move(other._eventHandlers) }
	{
		_eventHandlers.updateThis(this);
	}

	bool animated() const { return _animated; }
	bool updated() const { return _updated; }
	void setUpdated() { _updated = true; }
	void resetUpdated() { _updated = false; }
	auto& vars() { return _vars; }

	float eval(float x, float y);
	void update();
	void updateAnimated();
	void updateExpression(const std::string& expression);
};
}
